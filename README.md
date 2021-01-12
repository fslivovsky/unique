# Unique

*Unique* is a preprocessor for Quantified Boolean Formulas (given as [QDIMACS](http://www.qbflib.org/qdimacs.html) or [QCIR](http://fmv.jku.at/papers/JKS-BNP.pdf)) that extracts unique Skolem and Herbrand functions using propositional interpolation. See the [research paper](https://doi.org/10.1007/978-3-030-53288-8_24) for details.

## Installing

Unique is written in C++ and requires a compiler for C++17 and CMake (2.8.6 and later). Apart from that, requirements are inherited from [Avy](https://bitbucket.org/arieg/avy/src/master/). In particular, it requires [Boost](https://www.boost.org/) (1.46 or later).

Assuming all requirements are met, Unique can be downloaded and compiled using the following sequence of commands:

```git clone  https://github.com/perebor/unique.git
cd unique
git submodule init
git submodule update
mkdir build
cd build
cmake .. && make
```

## Usage

```
unique [options] <input file>
```
Here, the input file must be in [QDIMACS](http://www.qbflib.org/qdimacs.html) or (prenex) [QCIR](http://fmv.jku.at/papers/JKS-BNP.pdf).

A list of available options can be displayed using `--help`.

```
Usage: 
  unique [options] <input file>

Options:
  -h --help                     shows this screen
  -c --conflict-limit <int>     conflict limit for SAT solver (per variable) [default: 1000]
  -o --output-file <filename>   writes output to the given file (instead of standard output)
  -m --mode <mode>              determines which variables may be used in definitions [default: both]
                                (both | other-defined | other)
  --output-format <format>      Output format [default: QCIR]
                                (QCIR | QDIMACS | DIMACS | Verilog)
  --ordering-file <filename>    Read variable ordering for definability from file.               
```

By default, the interpolating SAT solver is run for a limited number of conflicts. To find unique Skolem/Herbrand functions of arbitrary complexity, use the option `--conflict-limit 0`.

The `--mode` option determines which variables may be used in a definition of a variable *x*. By default, both universal and existential variables preceding *x* may occur in a definition of *x*. With `--mode other`, definitions of existential variables may only use universal variables preceding *x*, and symmetrically, definitions of universal variables may only use existential variables preceding *x*. Using `--mode other-defined`, existential (universal) variables may additionally use preceding existential (universal) variables for which definitions have been found. This ensures that existential (universal) variables are defined exclusively in terms of universal (existential) variables, but may allow for shorter definitions.

Normally, Unique returns a QCIR file where definitions have been substituted for defined variables. The output format can be changed by  `--output-format` option. In particular, if `DIMACS` or `Verilog` are used, only an encoding of the definitions is returned.

By default, Unique uses the order of variables given in the input file. To override this, you can use the option `--ordering-file` to point to a text file that contains a single line with variable names in the order that they are supposed to be checked for definability.