SRC=$PWD/src
BUGGY=test.c
VULNERABLE=$PWD/vulnerable.txt


python3 ${DG_ROOT}/src/repair/main.py --src ${SRC} --buggy ${BUGGY} --vulnerable ${VULNERABLE} 
