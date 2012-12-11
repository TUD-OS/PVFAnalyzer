PVFAnalyzer
===========

Collection of tools for implementing Program Vulnerability Factor analysis.

* 'reader' -- parses an ELF file generating a CFG
* 'printer' -- prints a CFG into GraphViz format
* 'unroll' -- unrolls a CFG into an instruction trace
* 'dynrun' -- runs program and extends the CFG with runtime information, e.g., resolving dynamic jumps
* 'failtrace' -- converts a Fail/Bochs instruction trace into a PVF analyzable instruction list
* 'pvfrun' -- performs PVF analysis on an instruction trace