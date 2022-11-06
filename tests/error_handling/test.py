import z3.z3
import sys
print (sys.path)
z3.z3.parse_smt2_file('.dg/backend/klee-last/test000002.ktest')
