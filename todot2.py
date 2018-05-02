#!/usr/bin/python3
import sys

fileV=sys.argv[1]

print("graph jmeno{")

for l in open(fileV,'rt').readlines():
    if not l.strip(): continue
    k=l.split()
#    if k[0]=="Nodes":
#        for i in range(1,k[2]):
#            print()
    if k[0]!="VALUE":
        print(k[0]+" -- "+k[1])
print("}")
