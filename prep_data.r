
load("inp/initForFit_rf_0.31.rdata")

cn = colnames(datSel)
cn[cn == "st0"] = "initial"
cn[cn == "st1"] = "final"
cn[cn == "ENV1"] = "env1"
cn[cn == "ENV2"] = "env2"
cn[cn == "EB"] = "prevalenceB"
cn[cn == "ET"] = "prevalenceT"
cn[cn == "EM"] = "prevalenceM"
cn[cn == "itime"] = "interval"

colnames(datSel) = cn
write.table(datSel, "inp/trans_0.3.txt", sep=",", row.names=FALSE)

params = data.frame(
	name = names(params), 
	initialValue = params,
	samplerVariance = 0.4,
	priorMean = 0,
	priorSD = 10000,
	priorDist = "Normal")
write.table(params, "inp/inits_0.3.txt", sep=",", row.names=FALSE)


# ./bin/stm4_mcmc -p inp/inits_0.3.txt -t inp/trans_0.3.txt -n 1 -i 2000 -b 0 -c 2 -v 3