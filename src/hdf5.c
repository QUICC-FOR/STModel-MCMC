// compile:
// clang hdf5.c -lhdf5 
// OR
// h5cc -o test hdf5.c

#include <hdf5.h>
#include <stdio.h>

#define POSTERIOR_DIMS 2

	// some variables for testing just to avoid magic numbers
#define N_PARS 3
#define BUFF_SIZE 20

void make_posterior_dataset(hid_t dest, const char * name, int numParams, int bufferSize);
void add_posterior_samples(hid_t dataset, void * data, int bufferSize);

int main() {

	hid_t fileID;   // file identifier
	hid_t dataset;
	const char * filename = "testFile.h5";
	herr_t status;



	/* Create a new file using default properties. */
	// H5F_ACC_TRUNC means the file will be overwritten if it exists
	// the other two parameters are properties for file creation and access; defaults are fine
	fileID = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
	make_posterior_dataset(fileID, "testDat", N_PARS, BUFF_SIZE);

	// generate some data
	double dat1 [BUFF_SIZE][N_PARS];
	double dat2 [BUFF_SIZE][N_PARS];
	double dat3 [BUFF_SIZE][N_PARS];
	for(int rep = 0; rep < BUFF_SIZE; rep++) {
		for(int param = 0; param < N_PARS; param++) {
			dat1[rep][param] = param + rep*N_PARS;
			dat2[rep][param] = param + rep*N_PARS + N_PARS*BUFF_SIZE;
			dat3[rep][param] = param + rep*N_PARS + 2*N_PARS*BUFF_SIZE;
		}
	}
	
	// open the dataset
	dataset = H5Dopen(fileID, "testDat", H5P_DEFAULT);
	
	// write some data
	add_posterior_samples(dataset, dat1, BUFF_SIZE);
	add_posterior_samples(dataset, dat2, BUFF_SIZE);
	add_posterior_samples(dataset, dat3, BUFF_SIZE);
	
	// close the dataset
	H5Dclose(dataset);

	/* close the file. */
	status = H5Fclose(fileID); 
	
}

void make_posterior_dataset(hid_t dest, const char * name, int numParams, int bufferSize)
{
	hid_t dataspaceID, datasetID;

	/* first dimension (rows) are samples, 
	columns are the parameters within samples
	*/
	hsize_t startDim[POSTERIOR_DIMS] = {0, numParams};
	hsize_t maxDim[POSTERIOR_DIMS] = {H5S_UNLIMITED, numParams};
	
	dataspaceID = H5Screate_simple(POSTERIOR_DIMS, startDim, maxDim);
	
	/*
	the dataset will be chunked to improve I/O performance and allow for growth
	each chunk is B rows by K columns, where K is the total number of
	parameters and B is the size of the read/write buffer in the sampler object
	this allows single chunks to be written at once with each write
	*/
	// here we set creation properties to enable chunking
	hsize_t chunkDim[POSTERIOR_DIMS] = {bufferSize, numParams};
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


void add_posterior_samples(hid_t dataset, void * data, int bufferSize)
{

	// first extend the dataset
	hid_t filespace = H5Dget_space(dataset);
	hsize_t currentDims [POSTERIOR_DIMS];
	H5Sget_simple_extent_dims(filespace, currentDims, NULL);
	hsize_t newDims [POSTERIOR_DIMS] = {currentDims[0] + bufferSize, currentDims[1]};
	H5Dset_extent(dataset, newDims);

	// select hyperslab
	hsize_t slabStart [POSTERIOR_DIMS] = {currentDims[0], 0};
	hsize_t offsets [POSTERIOR_DIMS] = {bufferSize, currentDims[1]};
	H5Sclose(filespace);
	filespace = H5Dget_space(dataset);
	H5Sselect_hyperslab(filespace, H5S_SELECT_SET, slabStart, NULL, offsets, NULL);
		
	// make a dataspace for the buffer in memory
	hid_t memspace = H5Screate_simple(POSTERIOR_DIMS, offsets, NULL); 

	H5Dwrite(dataset, H5T_NATIVE_DOUBLE, memspace, filespace, H5P_DEFAULT, data);
	
	// close resources
	H5Sclose(filespace);
	H5Sclose(memspace);
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