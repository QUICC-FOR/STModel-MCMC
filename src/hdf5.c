// compile:
// clang hdf5.c -lhdf5 
// OR
// h5cc -o test hdf5.c

#include <hdf5.h>
#include <stdio.h>

#define POSTERIOR_DIMS 3

void make_posterior_dataset(hid_t dest, const char * name, int numParams, int numChains, 
		int bufferSize);
void add_posterior_samples(hid_t dataset, void * data, int bufferSize, hsize_t chainNo);

int main() {

	hid_t fileID;   // file identifier
	hid_t dataset;
	const char * filename = "testFile.h5";
	herr_t status;


	/* Create a new file using default properties. */
	// H5F_ACC_TRUNC means the file will be overwritten if it exists
	// the other two parameters are properties for file creation and access; defaults are fine
	fileID = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
	make_posterior_dataset(fileID, "testDat", 3, 2, 20);

	// generate some data
	double chain1 [20][3];
	double chain2 [20][3];
	for(int rep = 0; rep < 20; rep++) {
		for(int param = 0; param < 3; param++) {
			chain1[rep][param] = param + rep*3;
			chain2[rep][param] = param + rep*3 + 60;
		}
	}
	
	//       HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HER 
	//     HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE 
	//    HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE H
	//   HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HE
	// HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE
	//   HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HE
	//    HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE H
	//     HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE 
	//       HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HER 	
	
	// this doesn't work, because the code stupidly extends the dataset each time we add a chain
	// need to rethink this whole thing
	// I think the best thing to do is to treat the chains as totally different datasets
	// within a single group
	// this way each chain gets its own 2-d array, and we don't need to set aside space
	// that we might not need if chains are of varying length
	// could set it up using concurrent access; then have multiple processes (one per chain)
	
	// open the dataset
	dataset = H5Dopen(fileID, "testDat", H5P_DEFAULT);
	
	// write some data
	add_posterior_samples(dataset, chain1, 20, 0);
	add_posterior_samples(dataset, chain2, 20, 1);
	
	// close the dataset
	H5Dclose(dataset);

	/* close the file. */
	status = H5Fclose(fileID); 
	
}

void make_posterior_dataset(hid_t dest, const char * name, int numParams, int numChains, 
		int bufferSize)
{
	hid_t dataspaceID, datasetID;

	/* note the indexing of the dimensions. it is done to improve performance due to the
	layout of the array in memory and on disk. first dimension is chain (the slowest to
	change), the second is samples within chains, and the last is parameters within
	samples
	*/
	hsize_t startDim[POSTERIOR_DIMS] = {numChains, 0, numParams};
	hsize_t maxDim[POSTERIOR_DIMS] = {numChains, H5S_UNLIMITED, numParams};
	
	dataspaceID = H5Screate_simple(POSTERIOR_DIMS, startDim, maxDim);
	
	/*
	the dataset will be chunked to improve I/O performance and allow for growth
	each chunk is 1 chain, B rows, and K pars, where K is the total number of
	parameters and B is the size of the read/write buffer in the sampler object
	this allows single chunks to be written at once with each write
	*/
	// here we set creation properties to enable chunking
	hsize_t chunkDim[POSTERIOR_DIMS] = {1, bufferSize, numParams};
	hid_t plist = H5Pcreate(H5P_DATASET_CREATE);
	herr_t status = H5Pset_chunk (plist, POSTERIOR_DIMS, chunkDim);
	
	// create the dataset
	datasetID = H5Dcreate2(dest, name, H5T_NATIVE_DOUBLE, dataspaceID, H5P_DEFAULT, plist,
				H5P_DEFAULT);
				
	// close resources
	status = H5Dclose(datasetID);
	status = H5Pclose(plist);
	status = H5Sclose(dataspaceID);
}


void add_posterior_samples(hid_t dataset, void * data, int bufferSize, hsize_t chainNo)
{
	// first extend the dataset
	hid_t dataspace = H5Dget_space(dataset);
	hsize_t maxDims [POSTERIOR_DIMS];
	H5Sget_simple_extent_dims(dataspace, maxDims, NULL);
	hsize_t bufferStart [POSTERIOR_DIMS] = {chainNo, maxDims[1], 0};
	maxDims[1] += bufferSize;
	H5Dset_extent(dataset, maxDims);
	
	// some debugging
	printf("starting location: %lld, %lld, %lld\n", bufferStart[0], bufferStart[1], bufferStart[2]);
	printf("new maxDims: %lld, %lld, %lld\n", maxDims[0], maxDims[1], maxDims[2]);
	hsize_t testDims [POSTERIOR_DIMS];
	dataspace = H5Dget_space(dataset);
	H5Sget_simple_extent_dims(dataspace, testDims, NULL);
	printf("testDims: %lld, %lld, %lld\n", testDims[0], testDims[1], testDims[2]);
	
	// now select the hyperslab for appending
	hid_t filespace = H5Dget_space(dataset);
	hsize_t offsets [POSTERIOR_DIMS] = {1, bufferSize, maxDims[2]};
	H5Sselect_hyperslab(filespace, H5S_SELECT_SET, bufferStart, NULL, offsets, NULL);
//	hid_t memspace = H5Screate_simple(POSTERIOR_DIMS, extendDims, NULL); 
	printf("offsets: %lld, %lld, %lld\n", offsets[0], offsets[1], offsets[2]);

	// write the data
	H5Dwrite(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, filespace, H5P_DEFAULT, data);
	
	// close resources
	H5Sclose(dataspace);
	H5Sclose(filespace);
//	H5Sclose(memspace);
}

/*
steps to create a new MCMC dataset
	1. create file
	2. create all datasets
			- tuning parameters (k by j array)
			- input data
			- posterior samples - k by n by j array; will be empty initially
				k is number of parameters
				n is number of samples (should be variable/appendable)
				j is number of chains
			- metadata
				• thinning size
				• number iterations completed
				• starting conditions (k by j array)
				• number chains
				• RNG seed used
				• RNG type
	3. close file/wait for data


steps to write samples to existing dataset


steps to resume sampler from an existing dataset



*/