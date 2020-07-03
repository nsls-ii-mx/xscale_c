XSCALE_C_PAR
============

(C) Copyright 2019 Maciej R. Wlodek
7 June 2019

xscale_c_par runs xscale on all permutations or 
combinations of input data sets and produces a 
sorted table with completeness, r value, and 
number of unique reflections from each run.

Usage: xscale_c_par [-c | -p | -b] XSCALE.INP
Note:  use no more than 32 input files

Options:

  |---------------------|---------------------------------|
  | -h, --help:         |  Print usage message            |
  | -c, --combinations: |  Run xscale on all combinations |
  |                     |    of the input files           |
  | -p, --permutations: |  Run xscale on all permutations |
  |                     |    of the input files           |
  | -b, --both:         |  Run xscale on all combinations,|
  |                     |    and on those permutations    |
  |                     |    that swap the primary        |
  |                     |    (reference) input file       |
  |---------------------|---------------------------------|

xscale is an XDS program [Kabsch, Wolfgang. "XDS." Acta Cryst.
D66:2 (2010): 125 -- 132]  See

http://xds.mpimf-heidelberg.mpg.de/html_doc/xscale_parameters.html

for XSCALE.INP syntax

Comilation:

cc -o xscale_c_par xscale_c_par.c

