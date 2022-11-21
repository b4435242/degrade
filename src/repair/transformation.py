import subprocess
import sys
from os.path import join, exists, relpath, basename, realpath
import os
from utils import cd
import logging
import re
logger = logging.getLogger(__name__)


class VulnerableTransformer():
    def __init__(self, vulnerable, config, extracted):
        self.vulnerable = vulnerable
        self.config = config
        self.extracted = extracted
        if self.config.verbose:
            self.subproc_output = sys.stderr
        else:
            self.subproc_output = subprocess.DEVNULL
        

    def __call__(self, project):
        src = basename(project.dir)
        logger.info('instrumenting suspicious of {} source'.format(src))
        environment = os.environ
        environment['ANGELIX_EXTRACTED'] = self.extracted



        with cd(project.dir):
            argv = ['instrument-suspicious', project.buggy]
            argv += self.read_vulnerables()
            logger.info('argv {}'.format(argv))
            subprocess.call(argv,
                        stderr=self.subproc_output,
                        stdout=self.subproc_output,
                        env=environment)
    
    def read_vulnerables(self):
        with open(self.vulnerable, 'r') as f:
            locs = re.split('\s', f.read())
        return locs
                
