import argparse
import os
from os.path import join, exists, relpath, basename, realpath
import shutil
import stat
import statistics
import time
import logging

from transformation import RecordTransformer
from project import Frontend, Validation

from utils import format_time, time_limit, TimeoutException

logging.basicConfig(filename="main.log", level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s', datefmt='%Y-%m-%d %H:%M:%S')
logger = logging.getLogger(__name__)



class DG_RECORD:
    def __init__(self, working_dir, src, record, locations, build, configure, config):
        self.working_dir = working_dir
        self.config = config

        

        self.instrument_for_recording = RecordTransformer(locations, config)

        validation_dir = join(working_dir, "validation")
        shutil.copytree(src, validation_dir, symlinks=True)
        self.validation_src = Validation(config, validation_dir, record, build, configure)
        self.validation_src.configure()
        #self.validation_src.build()
        compilation_db = self.validation_src.export_compilation_db()

        frontend_dir = join(working_dir, "frontend")
        shutil.copytree(src, frontend_dir, symlinks=True)
        self.frontend_src = Frontend(config, frontend_dir, record, build, configure)
        self.frontend_src.import_compilation_db(compilation_db)
        self.frontend_src.initialize()
        
        #self.frontend_src.build()




    def generate_record(self):
        self.instrument_for_recording(self.frontend_src)
        self.frontend_src.configure()
        self.frontend_src.build()
       
if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    
    parser.add_argument('--src', metavar='SOURCE', help='source directory')
    parser.add_argument('--record', metavar='RECORD', help='relative path to record file')
    parser.add_argument('--locations', help='file specifying locations of recording statements')
    parser.add_argument('--build', default='make -e') # -e with modified environment
    parser.add_argument('--configure', default='./configure')
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
   
    args.build += ' -e' #make with env

    tool = DG_RECORD(working_dir, args.src, args.record, args.locations, args.build, args.configure, args)
    tool.generate_record()


    end = time.time()
    elapsed = format_time(end - start)

    logger.info("patch generated in {}".format(elapsed))
