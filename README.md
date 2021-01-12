# Unique

*Unique* is a preprocessor for Quantified Boolean Formulas (given as [QDIMACS](http://www.qbflib.org/qdimacs.html) or [QCIR](http://fmv.jku.at/papers/JKS-BNP.pdf)) that extracts unique Skolem and Herbrand functions using propositional interpolation.

## Installing
Unique is written in C++ and requires a compiler for C++17 and `CMake` (2.8 and later). Apart from that, requirements are inherited from [Avy](https://bitbucket.org/arieg/avy/src/master/)

```git clone  https://github.com/perebor/unique.git
cd unique
git submodule init
git submodule update
mkdir build
cd build
cmake .. && make
```

## Usage

