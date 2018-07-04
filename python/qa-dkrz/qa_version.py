import sys
import os

import argparse
import ConfigParser
import subprocess
from   types import *

from qa_config import CfgFile

import qa_util

class GetVersion(object):
    '''
    classdocs
    '''
    def __init__(self, extern=False, opts={}, com_line_opts={}):
        self.opts = opts
        if extern:
            self.opts.update( commandLineOpts( create_parser() ) )
            self.construct_cfg()
        else:
            self.opts.update( com_line_opts )

        self.p_projects=''

    def construct_cfg(self):
        if not "SECTION" in self.opts:
            sys.exit(1)

        self.opts["QA_SRC"] = self.opts["SECTION"]

        home = os.path.join( os.environ['HOME'], '.qa-dkrz')
        self.cfg_file=os.path.join(home, 'qa.cfg')
        self.cfg = CfgFile()
        self.cfg.read_file( self.cfg_file, section=self.opts["SECTION"])
        self.opts.update( self.cfg.getOpts() )

        return


    def get_external_version(self, prj):
        if self.isOpt("SHOW_VERSION"):
            sep0='\n'
            sep1='\n   '
        else:
            sep0='|'
            sep1=''

        if len(self.p_projects) == 0:
            self.p_projects = os.path.join(self.opts["QA_TABLES"], "tables", "projects")
        vStr=''

        if prj == "CMIP6":
            f=os.path.join(self.p_projects, prj, "CMIP6_CVs")
            if not os.path.isdir(f):
                self.no_such_table(prj, f)

            branch, curr_id = self.get_git_branch(f)
            vStr  = sep0 + "CMIP6_CVs:" + sep1
            vStr += branch + '-' + curr_id

            f=os.path.join(self.p_projects, prj, "CMIP6_MIP_tables.xlsx")
            if not os.path.isfile(f):
                self.no_such_table(prj, f)

            cmd="ls -l --time-style='+%F' " + f + "| awk '{print $6}'"

            try:
                t = subprocess.check_output(cmd,
                                            shell=True)
            except subprocess.CalledProcessError as e:
                istatus = e.returncode
            else:
                vStr += sep0 + "CMIP6_MIP_tables.xlsx:" + sep1
                vStr += t.strip()

            if self.isOpt("PrePARE"):
                p = self.opts["PrePARE"]
                p = qa_util.rstrip(p, sep='/', max=2)

                f = os.path.join(p, "conda-meta", "cmor-*")
                cmd="ls " + f + " 2> /dev/null"
                try:
                    t = subprocess.check_output(cmd,
                                                shell=True)
                except subprocess.CalledProcessError as e:
                    istatus = e.returncode
                else:
                    t = qa_util.lstrip(t, sep='/', pat='##')
                    t = qa_util.rstrip(t, sep='.')
                    vStr += sep0 + "CMOR:" + sep1 + t

                f=os.path.join(self.p_projects, prj, "cmip6-cmor-tables")
                if not os.path.isdir(f):
                    self.no_such_table(prj, f)

                branch, curr_id = self.get_git_branch(f)
                vStr += sep0 + "cmip6-cmor-tables:" + sep1 + branch + '-' + curr_id

        elif prj == "CORDEX":
            f=os.path.join(self.p_projects, prj, "IS-ENES-Data.github.io")
            if not os.path.isdir(f):
                self.no_such_table(prj, f)

            branch, curr_id = self.get_git_branch(f)

            vStr  = sep0 + prj + "/IS-ENES-Data.github.io:" + sep1
            vStr += branch + '-' + curr_id
        elif prj == "CF":
            if self.isOpt("CF_STD_NAME_VERSION"):
                vStr  = sep0 + "CF_STD_NAME_VERSION:" + sep1
                vStr += self.opts["CF_STD_NAME_VERSION"]
            else:
                f=os.path.join(self.p_projects, prj, "standard-names.html")
                if not os.path.isfile(f):
                    self.no_such_table(prj, f)

                if os.path.isfile(f):
                    with open(f, 'r') as fd:
                        for line in fd:
                            if 'Standard Name Table' in line:
                                p0 = line.find('(') + 1
                                p1 = line.find(')')
                                if p0 > 1 and p1 > -1:
                                    s = line[p0:p1].split()

                                    vStr  = sep0 + "CF_STD_NAME_VERSION:" + sep1
                                    for item in s:
                                        vStr += item

                                    break

        return vStr


    def get_git_branch(self, path):
        branch=''
        curr_id=''

        #cmd="git branch | grep '*' | awk '{print $2}'"
        #cmd="git -C " + path + " branch"
        cmd="git branch"

        try:
            branch = subprocess.check_output(cmd,
                                             cwd=path,
                                             shell=True)
        except subprocess.CalledProcessError as e:
            istatus = e.returncode
        else:
            branch = branch.split()
            for i in range(len(branch)):
                if branch[i] == '*':
                    branch = branch[i+1]

        cmd="git log --pretty=format:'%h' -n 1"

        try:
            curr_id = subprocess.check_output(cmd,
                                              shell=True,
                                              cwd=path)
        except subprocess.CalledProcessError as e:
            istatus = e.returncode

        return branch, curr_id


    def get_from_conda(self):
        tag='-'
        branch='-'
        curr_id='-'

        f = os.path.join(self.opts["QA_SRC"], "install.log")
        cmd="cat " + f + " 2> /dev/null"
        try:
            t = subprocess.check_output(cmd,
                                        shell=True)
        except subprocess.CalledProcessError as e:
            istatus = e.returncode
        else:
            t = t.split('\n')
            branch = qa_util.lstrip(t[0], 'branch=')
            curr_id = qa_util.lstrip(t[1], 'hexa=')

            if len(t) > 2:
                if 'tag' in t[2]:
                    tag = qa_util.lstrip(t[2], 'tag=')

        return branch, curr_id, tag


    def get_QA_version(self):
        tag=''

        if self.isOpt("CONDA"):
            branch, id, tag = self.get_from_conda()
        else:
            branch, id = self.get_git_branch(self.opts["QA_SRC"])

            f=os.path.join(self.opts["QA_SRC"], "conda-recipes", "qa-dkrz", "meta.yaml")
            if os.path.isfile(f):
                with open(f, 'r') as fd:
                    for line in fd:
                        if 'name:' in line:
                            words=line.split()
                            tag=words[-1]
                        elif 'version:' in line:
                            words=line.split()
                            tag += '-' + words[-1].strip("'")
                        elif 'number:' in line:
                            words=line.split()
                            tag += '-' + words[-1]
                            break


        if self.isOpt("SHOW_VERSION"):
            rev = "QA version:"
            if len(tag):
                rev += '\n   tag: ' + tag

            rev += '\n   branch: ' + branch
            rev += '\n   commit-SHA: ' + id
        else:
            rev=''
            if len(tag):
                rev = tag + ','
            rev += branch + '-' + id

        return rev


    def isOpt(self, key, dct={}):
        # a) an option exists, then return True, but,
        # b) if the type is bool, then return the value
        if len(dct):
            curr_dct = dct
        else:
            curr_dct = self.opts

        if key in curr_dct:
            val = curr_dct[key]

            if type(val) == BooleanType:
               return val
            else:
               if not val == 'f':
                    return True

        return False


    def no_such_table(self, prj, path):
        print 'Missing external tables for project ' + prj
        print 'Is ' + path + ' installed?'
        print 'If not, then run: install --up [--force] [--freeze] ' + prj

        return


    def read_file(self, file, section=''):

        self.section = section

        if len(file):
            self.file = file

        # any valid conf-file written by a ConfigParser?
        (path, tail)=os.path.split(self.file)

        if os.path.isfile(self.file):
            # read a file created by a self.rCP instance
            self.rCP.read(self.file)
            self.isModified=False

        else:
            old = os.path.join(path, 'config.txt' )

            if os.path.isfile(old):
            # note config.txt is a file not created  by self.rCP,
            # conversion.
                with open(old) as r:
                    self.isModified=True

                    for line in r:
                        line = qa_util.clear_line(line)
                        if len(line):
                            sz1=len(line)-1

                            if line[sz1] == ':' :
                                ny_sect=line[0:sz1] # rm ':'
                                self.rCP.add_section(ny_sect)
                            else:
                                k, v = line.split('=')
                                self.rCP.set(ny_sect, k, repr(v))
        return

    def run(self):
        # version of QA-DKRZ
        revision = self.get_QA_version()

        if not self.isOpt('PROJECT'):
            return revision

        # versions of external projects and tables
        lprj = []

        if not "CF" in self.opts["PROJECT"]:
            lprj.append("CF")


        if self.isOpt("PROJECT"):
            # convert string to []
            if type(self.opts["PROJECT"]) == StringType:
                lprj.append( self.opts["PROJECT"] )
            else:
                lprj.extend( self.opts["PROJECT"].split() )

        for prj in lprj:
            s = self.get_external_version(prj)
            if len(s):
                revision += s

        return revision



def commandLineOpts(parser):

    # let's parse
    args = parser.parse_args(sys.argv[1:])

    # post-processing: namespace --> dict
    _ldo = {}

    if args.is_conda:            _ldo['CONDA']      = True
    if args.CONFIG_FILE:         _ldo['CONFIG_FILE']   = args.CONFIG_FILE
    if args.is_get_branch:       _ldo['GET_BRANCH']    = True
    if args.PROJECT    != None:  _ldo['PROJECT']       = args.PROJECT.split(',')
    if args.SECTION    != None:  _ldo['SECTION']       = args.SECTION
    if args.SHOW_VERSION:        _ldo['SHOW_VERSION']  = True

    return _ldo


def create_parser():
    parser = argparse.ArgumentParser(
            prog="qa_version",
            description="Return a string with versions of QA.DKRZ and required external tables and programs."
            )

    parser.add_argument( '-P', '--project', dest='PROJECT',
        help="Name of the project(s); multiples are comma-separated")

    parser.add_argument( '-S', '--section', dest='SECTION',
        required=True, help="Section of ~/.qa-dkrz/qa.cfg")

    parser.add_argument( '--conda', dest='is_conda',
        action="store_true",
        help="For the qa-dkrz instance installed by conda")

    parser.add_argument( '--config-file', dest='CONFIG_FILE',
        help="The config-file of QA-DKRZ")

    parser.add_argument( '--get-branch', dest='is_get_branch',
        action="store_true",
        help="The config-file of QA-DKRZ")

    parser.add_argument( '--verbose' , '-v', dest='SHOW_VERSION',
        action="store_true", help="verbose")

    return parser


def getOpt(self, key, dct={}, bStr=False):
    if len(dct):
        curr_dct = dct
    else:
        curr_dct = self.dOpts

    if key in curr_dct:
        val = curr_dct[key]

        if bStr:
            val = str(val)

        return val

    return ''

def get_version(extern=False, opts={}, com_line_opts={}):
    # 1st param: CfgFile object, imported from qa_config
    # 2nd param: command-line options {}
    gv = GetVersion(extern=False, opts=opts, com_line_opts=com_line_opts)
    rev = gv.run()

    return rev



if __name__ == '__main__':
    sys.argv.append("--verbose")

    rev = get_version(extern=True)
    print rev
