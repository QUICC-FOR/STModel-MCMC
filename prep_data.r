
load("inp/initForFit_rf_0.05.rdata")
cn = colnames(datSel)
cn[cn == "st0"] = "initial"
cn[cn == "st1"] = "final"
cn[cn == "ENV1"] = "env1"
cn[cn == "ENV2"] = "env2"
cn[cn == "EB"] = "expectedB"
cn[cn == "ET"] = "expectedT"
cn[cn == "EM"] = "expectedM"
cn[cn == "itime"] = "interval"

colnames(datSel) = cn
write.table(datSel, "inp/trans_0.05.txt", sep=",", row.names=FALSE)

params = read.table("inp/GenSA_initForFit_rf_0.05.txt", header=FALSE, skip=1)
colnames(params) = c("name", "initialValue")
params$priorMean = rep(0, nrow(params))
params$priorSD = rep(10000, nrow(params))
write.table(params, "inp/inits_0.05.txt", sep=",", row.names=FALSE)