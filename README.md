# Pythia8ProIO

This is an example of how to interface Pythia8 Monte Carlo 
generator with the [ProIO file format](https://github.com/proio-org) for  
[HepSim Monte Carlo repository](http://atlaswww.hep.anl.gov/hepsim/).
We use the VarintPackedParticles method to fill particle records which gives very good file size
reduction. 

It assumes that the following variables are set:

```
LZ4
PROIO
PROTOBUF
PYTHIA8
CLHEP
```

Look at the Makefile. 
After compiling the code, run the test job "A_RUN" which creates 10 ProIO files inside the directory "out"


S.Chekanov (ANL)


