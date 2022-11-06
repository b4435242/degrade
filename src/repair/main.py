import argparse
import os
from os.path import join, exists, relpath, basename, realpath
import shutil
import stat
import statistics
import time
import logging

from transformation import VulnerableTransformer
from project import Frontend, Backend, Validation
from testing import Tester
from inference import Inferrer
from utils import format_time, time_limit, TimeoutException

logging.basicConfig(filename="main.log", level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s', datefmt='%Y-%m-%d %H:%M:%S')
logger = logging.getLogger(__name__)


class DG:
    def __init__(self, working_dir, src, buggy, vulnerable, build, configure, config):
        
        self.instrument_for_inference = VulnerableTransformer(vulnerable, config)
        self.tester = Tester(config, working_dir)
        self.inferrer = Inferrer(config, self.tester)

        validation_dir = join(working_dir, "validation")
        shutil.copytree(src, validation_dir, symlinks=True)
        self.validation_src = Validation(config, validation_dir, buggy, build, configure)
        self.validation_src.configure()
        compilation_db = self.validation_src.export_compilation_db()
        self.validation_src.import_compilation_db(compilation_db)
        #self.validation_src.initialize()
        
        frontend_dir = join(working_dir, "frontend")
        shutil.copytree(src, frontend_dir, symlinks=True)
        self.frontend_src = Frontend(config, frontend_dir, buggy, build, configure)
        self.frontend_src.import_compilation_db(compilation_db)
        #self.frontend_src.initialize()

        backend_dir = join(working_dir, "backend")
        shutil.copytree(src, backend_dir, symlinks=True)
        self.backend_src = Backend(config, backend_dir, buggy, build, configure)    
        self.backend_src.import_compilation_db(compilation_db)
        self.backend_src.initialize()
        self.backend_src.build() 




    def generate_patch(self):
        self.instrument_for_inference(self.backend_src)
        self.backend_src.build()
        self.inferrer(self.backend_src)
    

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    
    parser.add_argument('--src', metavar='SOURCE', help='source directory')
    parser.add_argument('--buggy', metavar='BUGGY', help='relative path to buggy file')
    parser.add_argument('--vulnerable', help='file specifying addresses of vulnerable statements')
    parser.add_argument('--build', default='make -e') # -e with modified environment
    parser.add_argument('--configure', default=None)
    parser.add_argument('--verbose', default=True)
    parser.add_argument('--mute_build_message', default=True)
    parser.add_argument('--mute_warning', default=True)

    args = parser.parse_args()


    
    def rm_force(action, name, exc):
        os.chmod(name, stat.S_IREAD)
        shutil.rmtree(name)
    working_dir = join(os.getcwd(), ".dg")
    if exists(working_dir):
        shutil.rmtree(working_dir, onerror=rm_force)
    os.mkdir(working_dir)
    logger.info("working_dir {}".format(working_dir))


    start = time.time()
    
    tool = DG(working_dir, args.src, args.buggy, args.vulnerable, args.build, args.configure, args)
    tool.generate_patch()
    


    end = time.time()
    elapsed = format_time(end - start)

    logger.info("patch generated in {}".format(elapsed))
