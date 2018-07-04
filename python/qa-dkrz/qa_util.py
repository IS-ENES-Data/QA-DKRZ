'''
Created on 21.03.2016


@author: hdh
'''

import copy
import glob
import hashlib
import keyword
import os
import re
import shutil
import subprocess
import sys
import threading
import time

from types import *

class GetPaths(object):
    '''
    classdocs
    '''
    def __init__(self, qaConf):
        dOpts = qaConf.dOpts
        self.dOpts = dOpts

        # list of lists containing sub-paths to selected data sets.
        # Paths are split into a leading base path component and a sub-path.
        # The index of the base path points to a list of base paths.

        self.root=[]
        self.fBase=[]
        self.fNames=[]

        prj_data_path = qaConf.getOpt('PROJECT_DATA')

        self.is_only_ncfiles = False if 'QUERY_NON_NC_FILE' in dOpts else True
        self.is_empty_dir    = False if 'QUERY_EMPTY_DIR'   in dOpts else True

        # SELECTed variables and paths as well as the LOCKed counterpart
        # may contain several RegExp expressions.

        # SELECT
        self.selVar  = copy.deepcopy(dOpts['SELECT_VAR_LIST'])
        self.selPath = copy.deepcopy(dOpts['SELECT_PATH_LIST'])

        # maximum non-regExp path to data for each SELECTion;
        # starting point for walking
        self.getBasePaths(prj_data_path)

        # LOCK
        self.lockVar  = dOpts['LOCK_VAR_LIST']
        self.lockPath = dOpts['LOCK_PATH_LIST']

        for i in range(len(self.lockPath)):
            self.lockPath[i] = os.path.join(prj_data_path, self.lockPath[i])

        self.curr_walk_ix = 0

        # instance of a generator function
        self.my_walk = os.walk(self.base_path[self.curr_walk_ix], followlinks=True)

#        self.isCycle=True
#        self.buffer_len=3

#        self.eventObj = threading.Event()
#        self.thrd = threading.Thread(target = self.find,
#                                args = (dOpts, self.eventObj,))

#        self.g = self.find()

    def __iter__(self):
        return self


    def applySelection(self, root, fbases):
        # test the path against selected paths coupled with variables.
        # selVar and selPath act pair-wise, and so is for lock
        ret_fbases = []

        for i in range(len(fbases)):
            fbase = fbases[i]
            isLocked=False

            # any lock extended?
            for j in range(len(self.lockPath)):
                isLocked=False

                lockPath = self.lockPath[j]
                lockVar  = self.lockVar[j]

                # is a path locked?
                if self.lockPath[j] == '.*':
                    isLocked = True
                else:
                    regExp = re.search(self.lockPath[j] , root)
                    if regExp:
                        isLocked = True

                # is a corresponding filename locked?
                if isLocked:
                    if self.lockVar[j] == '.*':
                        break
                    else:
                        regExp = re.search(self.lockVar[j] , fbase)
                        if regExp:
                            break
                        else:
                            isLocked = False

            if isLocked:
                continue

            isSelected=True

            selPath = self.selPath[self.curr_walk_ix]
            selVar  = self.selVar[self.curr_walk_ix]

            # test for a selected path
            if not (selPath == '.*' or selPath == ''):
                regExp = re.search(selPath , root)
                if not regExp:
                    isSelected = False

            # now the files
            # test the file name against selected variables
            if isSelected and selVar != '.*':
                regExp = re.search(selVar, fbase)
                if not regExp:
                    isSelected = False

            if isSelected:
                ret_fbases.append(fbase)

        return ret_fbases


    def getBasePaths(self, pd):
        base_path = []
        pp_ix=-1
        sz = len(self.selPath)
        if sz:
            for i in range(sz):
                base_path.append(pd)
                pp_len = len(base_path[i])
                pp_ix += 1

                # split into path components
                x_selPath = self.selPath[i].split('/')
                recomb=''
                isCont = True

                for item in x_selPath:
                    if not item:
                        if not base_path[pp_ix]:
                            base_path[pp_ix] = '/'

                        continue

                    if isCont:
                        re_obj = re.match(r'([a-zA-Z0-9_-]*)', item)

                        if re_obj:
                            if re_obj.group(0) == item:
                                # extend the base path
                                base_path[pp_ix] = os.path.join(
                                        base_path[pp_ix], item)
                            else:
                                # found item with a regExp (or glob) component
                                isCont=False
                                recomb = os.path.join(recomb, item)
                    else:
                        recomb = os.path.join(recomb, item)

                if len(base_path[i]) < pp_len:
                    # adjust the SELECTion path; it is shorter
                    self.selPath[i] = recomb
        else:
            h, t = os.path.split(pd)

            base_path.append(h)
            self.selPath = [t]
            self.selVar  = ['.*']

        self.base_path=[]
        for i in range(len(base_path)):
            if base_path[i]:
                if base_path[i][0:2] == './':
                    self.base_path.append(self.dOpts["CURR_DIR"]+base_path[i][1:])
                elif base_path[i][0:3] == '../':
                    str0 = rstrip(self.dOpts["CURR_DIR"], '/')
                    self.base_path.append(str0+base_path[i][2:])
                elif base_path[i][0] != '/':
                    # a relative path
                    self.base_path.append(self.dOpts["CURR_DIR"]+'/'+base_path[i])
                else:
                    self.base_path.append(base_path[i])
            else:
                self.base_path.append(self.dOpts["CURR_DIR"])

        return


    def getFilenameBase(self, path, fs):
        fBase=[]
        fNames=[]  # becomes a 2D list
        countUnreadableFiles = 0

        for f in fs:
            # check read permission
            if not os.access(os.path.join(path, f), os.R_OK):
                countUnreadableFiles += 1
                continue

            for i in range(len(fBase)):
                if f[0:len(fBase[i])] == fBase[i]:
                    fNames[i].append(f)
                    break
            else:
                if self.is_only_ncfiles:
                    if f[-3:] != '.nc':
                        # todo: annotation --> log-file
                        continue

                # t_r contains ( fBase, StartTime, EndTime )
                t_r = f_time_range(f)

                fBase.append(t_r[0])
                fNames.append([])
                fNames[len(fBase)-1].append(f)

        if countUnreadableFiles == len(fs):
            if countUnreadableFiles:
                print 'no read-permission for files in' + path
                sys.exit(1)

        return fBase, fNames


    def next(self):
        # list self.fBase could have len > 1
        while len(self.fBase):
            if len(self.fBase[0]):
                return self.root, self.fBase.pop(0), self.fNames.pop(0)
            else:
                self.fBase.pop[0]
                self.fNames.pop[0]

        if not (len(self.fBase) and len(self.fBase[0]) ):
            while True:
                try:
                    self.root, ds, fs = self.step_walk()
                except StopIteration:
                    # are there more SELECTions, i.e. also more
                    # paths to walk?
                    self.curr_walk_ix += 1
                    if self.curr_walk_ix < len(self.base_path):
                        self.my_walk = os.walk(self.base_path[self.curr_walk_ix])
                    else:
                        # reached the very end
                        raise StopIteration
                else:
                    if not os.access(self.root, os.R_OK):
                        self.countUnreadablePaths += 1

                    # Return a list of filename bases (usually one). Each fBase
                    # is associated with a list of filenames
                    self.fBase, self.fNames = self.getFilenameBase(self.root, fs)

                    # Check selection. If any fBase item is not selected, then
                    # its place in fBase is taken by an empty string.
                    self.fBase = self.applySelection(self.root, self.fBase)

                    while len(self.fBase):
                        if len(self.fBase[0]):
                            return self.root, self.fBase.pop(0), self.fNames.pop(0)
                        else:
                            self.fBase.pop(0)
                            self.fNames.pop(0)


    def step_walk(self):
        # walk recursively starting at the base path
        while True:
            root, ds, fs = self.my_walk.next()
            if len(fs) :
                return root, ds, fs



# ------------ plain functions ---------

def add_unique(arg, lst):
    if arg not in lst:
        lst.append(arg)

    return

def cat( srcs, dest, append=False):
    # convert a string into a list
    if type(srcs) == StringType:
        dsrcs=[srcs]
    else:
        dsrcs = copy.deepcopy(srcs)

    origDest=dest
    dest += '.' + repr(os.getpid())

    if append:
        dsrcs.insert(0, origDest)

    is_incomplete = False

    try:
        with open(dest, 'w') as fd:
            for src in dsrcs:
                try:
                    with open(src) as r:
                        for line in r:
                            fd.write(line)
                except:
                    # appending to a new destination file is ok
                    if not (append and origDest == src):
                        is_incomplete = True
                        break
    except:
        is_incomplete = True

    if is_incomplete:
        os.remove(dest)
        return False

    os.rename(dest, origDest)

    return True


def clear_line(line):
    line=line.replace(' ','')  # all blanks
    line = line.strip() # other white chars and \n

    pos = line.find('#')
    if pos > -1:
        line = line[0:pos]

    return line


def convert2brace(lst):
    if type(lst) == StringType:
        lst = lst.split(',')

    s=''
    for l in lst:
        l = l.strip()
        if l[0] == '{':
            l = '\\' + l

        if l[len(l)-1] == '}':
            l = l[0:-1] + '\\}'

        if len(s):
            s += ','

        s += l

    return s


def date(t=0):
    if t == 0:
        lt = time.localtime(time.time())

        # ISO
        dt = '{}-{}-{}T{}:{}:{}'.format(lt[0], \
            str(lt[1]).rjust(2,'0'), \
            str(lt[2]).rjust(2,'0'), \
            str(lt[3]).rjust(2,'0'), \
            str(lt[4]).rjust(2,'0'), \
            str(lt[5]).rjust(2,'0') )

    return dt


def f_get_mod_time(f):
    try:
        st=os.stat(f)
    except OSError:
        t=0
    else:
        t = st.st_mtime

    return t

def f_grep(rFile, item, opt_A=-1, opt_c=False, opt_i=False, opt_m=0,
           opt_q=False, opt_v=False):
    # item: str
    # see man-page for grep
    matches = []
    num = 0

    with open(rFile, 'r') as f:
        for line in f:
            regExp = re.search(item, line)

            if opt_v:
                regExp = not regExp

            if regExp:
                num += 1

                if not opt_c:
                    if opt_q:
                        return True

                    matches.append(line)

                    if opt_m == num:
                        break

    if opt_m:
        return num

    return matches


def f_time_range(f):
    # sub-temporal time ranges of files
    dts = []

    # removal of '.nc' extension is necessary for fixed variables
    if f[-3:] == '.nc':
        f = f[0:-3]

    re_obj = re.match(r'(.*)_(\d+)[-_]*(\d*)', f)

    if re_obj:
        # at least one date was found
        if len(re_obj.group(3)):
            # two dates provided
            return re_obj.groups()
        else:
            # two cases: just a single date or two wrongly separated by '_'
            re_obj = re.match(r'(.*)_(\d+)_(\d+)', f)
            if re_obj:
                if len(re_obj.groups()) == 3:
                    # wrongly separated
                    return re_obj.groups()
                else:
                    return ( re_obj.group(1), re_obj.group(2), re_obj.group(2) )

    return (f, '', '')  # no date at all


def f_str_replace(rFile, items, repls):
    # items: list or str
    # repls: list or str. If the number is identical, the used by pairs
    #        If number of items > 1 and number of repls == 1, then replace each
    #        item by repls[0]

    if type(items) == StringType:
        items=[ items ]

    num=len(items)

    if type(repls) == StringType:
        rpls=[repls]
        i=1
        while i < num:
            rpls.append(repls[0])
        repls=rpls

    if num != len(repls):
        print 'inappropriate args: items=' + repr(items) + ' vs. repl=' + repr(repls)

    rng = range(len(repls))
    wFile = rFile + repr(os.getpid())

    with open(rFile, 'r') as f:
        try:
            w = open(wFile, 'w')
        except IOError:
            print 'could not open file ' + wFile
            sys.exit(1)

        for line in f:
            # line = clear_line(line)
            if len(line):
                for ix in rng: # loop over items
                    if items[ix] in line:
                        line = line.replace(items[ix], repls[ix])

            w.write(line)

        w.close()
        os.rename(wFile, rFile)

        return


def get_curr_revision(src, is_CONDA):
    d={}

    if is_CONDA:
        with open( os.path.join(src,'install.log')) as f:
            lst=f.read().split()

        for l in lst:
            t0,t1=(l.split('='))
            d[t0]=t1
    else:
        # inquire the git branch
        try:
            lst=subprocess.check_output(["git",
                                          "--work-tree="+src,
                                          "--git-dir="+src+"/.git",
                                          "branch"
                                          ]).split('\n')
        except subprocess.CalledProcessError as e:
            print e.output()
            d['branch']='?'
            d['hexa']='?'
            return ''
        else:
            for s in lst:
                if s[0] == '*':
                    s = s.split()
                    d['branch'] = s[1]
                    break

        # inquire the git version of the branch
        try:
            s=subprocess.check_output(["git",
                                          "--work-tree="+src,
                                          "--git-dir="+src+"/.git",
                                          "log",
                                          "--pretty=format:'%h'",
                                          "-n",
                                          "1"
                                          ])
        except subprocess.CalledProcessError as e:
            print e.output()
            d['branch']='?'
            d['hexa']='?'
            return ''
        else:
            d['hexa'] = s

    return d['branch'] + '-' + d['hexa']


def get_experiment_name(g_vars, qaConf, fB='', sp='', isInit=False):

    if isInit:
        # precedence for provided LOG_NAME
        g_vars.log_fname = ''
        if qaConf.isOpt('LOG_FNAME'):
            g_vars.log_fname = qaConf.getOpt('LOG_FNAME')

            qaConf.delOpt('LOG_FNAME_INDEX')
            qaConf.delOpt('LOG_PATH_INDEX')

        elif qaConf.isOpt('LOG_PATH_INDEX'):
            g_vars.log_path_index = qaConf.getOpt('LOG_PATH_INDEX')
            qaConf.addOpt('LOG_INDEX_MAX', max(g_vars.log_path_index))

        elif qaConf.isOpt('LOG_FNAME_INDEX'):
            g_vars.log_fname_index = qaConf.getOpt('LOG_PATH_INDEX')
            qaConf.addOpt('LOG_INDEX_MAX', max(g_vars.log_fname_index))

        else:
            g_vars.log_fname = 'all-scope'
            qaConf.addOpt('LOG_NAME', g_vars.log_fname)

        g_vars.log_fnames = []

        return

    if len(g_vars.log_fname):
        curr_name = g_vars.log_fname
    elif len(g_vars.log_path_index):
        path = os.path.join(g_vars.project_data_path, sp)
        curr_name = get_name_from_path(path, g_vars.drs_path_base,
                                       g_vars.log_path_index)
    elif len(g_vars.log_fname_index):
        curr_name = get_name_from_file(fB, g_vars.log_fname_index)

    # collection of used log_fnames
    if not curr_name in g_vars.log_fnames:
        g_vars.log_fnames.append(curr_name)

    return curr_name


def get_md5sum(arg):
   md5 = hashlib.md5()

    # try at first for a list, then a file and eventually a string
   if type(arg) == ListType:
      for a in arg:
         md5.update(a)
   elif os.path.isfile(arg):
      with open(arg, 'rb') as fd:
         while True:
               s = fd.read(32768)
               if len(s):
                  md5.update(s)
               else:
                  break
   else:
      md5.update(arg)

   return md5.hexdigest()


def get_name_from_file(fBase, fname_index):
    #  extract the name of the experiment from filenames
    items = fBase.split('_')
    items_len = len(items)

    pIx = []
    for ix in fname_index:
        pIx.append(ix)

    # assemble the name
    name=''
    for ix in pIx:
        if len(name):
            name += '_'

        if ix < items_len:
            name += items[ix]
        else:
            name = 'unkownExp'
            break

    return name


def get_name_from_path(path, drs_path_base, path_index):
    #  extract the name of the experiment from subPath
    path_items = path.split('/')

    # rm empty items
    for c in range(len(path_items)-1,-1,-1):
        if len(path_items[c]) == 0:
            del path_items[c]

    path_items_len = len(path_items)

    # find the starting item with index == 0
    if len(drs_path_base) == 0:
        drs_path_base = path_items[0]

    # Take into account multiple occurrences.
    # If so, then take the right-most one
    base_ix=0
    for c in range(path_items_len):
        if path_items[c].lower() == drs_path_base.lower():
            base_ix = c

    pIx = []
    for ix in path_index:
        pIx.append( base_ix + ix )

    # assemble the name
    name=''
    for ix in pIx:
        if ix < path_items_len:
            if len(name):
                name += '_'

            name += path_items[ix]

    if len(name) == 0:
        name = 'unknownExp'

    return name


def get_project_table_name(g_vars, qaConf, fB='', sp='', isInit=False):

    if isInit:
        # identical to experiment_name

        # precedence for provided prefix of project tables
        g_vars.pt_name = ''
        g_vars.ct_path_index = []

        if qaConf.isOpt('PROJECT_TABLE'):
            g_vars.pt_name = qaConf.getOpt('PROJECT_TABLE')

            qaConf.delOpt('PT_PATH_INDEX')
            qaConf.delOpt('PROJECT_TABLE_PREFIX')
            g_vars.pt_prefix = ''

        elif qaConf.isOpt('CT_PATH_INDEX'):
            g_vars.ct_path_index = qaConf.getOpt('CT_PATH_INDEX')
            qaConf.addOpt('CT_PATH_INDEX_MAX', max(g_vars.ct_path_index))

            if len(g_vars.ct_path_index):
                g_vars.ct_path_index_max = max(g_vars.ct_path_index)
            else:
                g_vars.ct_path_index_max = 100

        else:
            g_vars.pt_name = 'pt_all-scope'
            qaConf.addOpt('PROJECT_TABLE', g_vars.pt_name)
            g_vars.pt_prefix = ''

        if qaConf.isOpt('PROJECT_TABLE_PREFIX'):
            g_vars.pt_prefix = qaConf.getOpt('PROJECT_TABLE_PREFIX') + '_'
        else:
            g_vars.pt_prefix = 'ct_'

        g_vars.pt_names = []

        return

    if len(g_vars.pt_name):
        curr_name = g_vars.pt_name

    elif len(g_vars.ct_path_index):
        path = os.path.join(g_vars.project_data_path, sp)
        curr_name = get_name_from_path(path, g_vars.drs_path_base,
                                       g_vars.ct_path_index)

    # collection of used log_fnames
    if not curr_name in g_vars.pt_names:
        g_vars.pt_names.append(curr_name)

    return g_vars.pt_prefix + curr_name



def get_sorted_options(opts, sep='='):
    mKeys = opts.keys()
    mKeys.sort()

    lst=[]
    for key in mKeys:
        lst.append('{}'.format(key) +sep+ '{}'.format(opts[key]) )

    return lst


def get_str_head(str0, sep='/'):
    return str0[0:str0.find(sep)]


def get_str_tail(str0, sep='/'):
    return str0[str0.rfind(sep)+1:]


def get_QA_SRC(path):
    # the root of QA_SRC contains
    #   a) scripts/qa-dkrz
    #   b) python/qa-dkrz/qa-dkrz.py

    # is argv[0] called by a plain name?
    head, tail = os.path.split(path)
    if len(head) == 0:
        target=os.path.join(os.getcwd(), tail)
        return get_QA_SRC(target)

    p, name = os.path.split(path)
    pp, tail_b = os.path.split(p)
    ppp, tail_p = os.path.split(pp)

    if not p[0] == '.':
        if tail_b == 'scripts':
            return (False, pp)
        elif tail_p == 'python':
            return (True, ppp)

    target=path

    if target[0] == '.' or '/.' in target:
        old_path=os.getcwd()
        os.chdir(p)     # remove . and .. anywhere in the path
        target=os.path.join(os.getcwd(), tail)
        os.chdir(old_path)

    if os.path.islink(target):
        target=os.readlink(target)

        # link is relative, so make it absolute
        if target[0] != '/':
            target = os.path.join(os.path.dirname(target),target)

        if not os.path.exists(target):
            print 'broken link ' + target
            sys.exit(1)
    elif not os.path.exists(target):
        print 'invalid path=' + target
        sys.exit(1)

    return get_QA_SRC(target)

    return (False, '')


def isValid_var_name(name):
    # test for a valid variable name
    name = re.sub('[^0-9a-zA-Z_]', '', name)  # rm invalid chars
    name = re.sub('^[^a-zA-Z_]+', '', name)   # rm invalid leading chars

    if not len(name):
        return False

    if keyword.iskeyword(name):
        return False

    return True


def join(val, sep=','):
   # In contrast to the str.join method: if val is not a list, but a single string,
   # then sep.join('asd') would not result in 'a,s,d'.
   # Additionally, a list of int is transformed to a csv string

    if type(val) == ListType:
       v=''
       for x in val:
          if v:
             v += sep

          if type(x) == IntType or type(x) == LongType:
             v += str(x)
          else:
             v += x

       return v

    return val


def mkdirP(path):

    # os.makedirs raises error when dirs already exist
    try:
        os.makedirs(path)
    except OSError:
        pass

    # test for a genuine failed making of dirs
    if not os.path.isdir(path):
        os.makedirs(path)
        return False

    return True


def mk_list(items):
   if type(items) == ListType:
       return items

   return [items]


def rm_r(*args):
    status=True
    for a in args:
        if os.path.isdir(a) or os.path.isfile(a):
            try:
                os.remove(a)
            except OSError:
                shutil.rmtree(a)
            except:
                pass

    return


def split(s,ssep, str_sep=False, remove_empty=True):
    # Split at a string (str_sep=True) or at the chars of
    # of a separation string (be default).
    # Empty items in the returned [] are removed by default.

    sep=[]
    if str_sep:
        sep.append(ssep)
    else:
        for x in ssep:
            sep.extend(x)

    splt1=s.split(sep[0])

    for i in range(1,len(sep)):
        splt0=splt1
        splt1=[]
        for x in splt0:
            splt1.extend(x.split(sep[i]))

    # remove empty items
    if remove_empty:
        for i in range(len(splt1)-1,-1,-1):
            if len(splt1[i]) == 0:
                del splt1[i]

    return splt1


def strip(s, sep=' '):
    s = strip_l(s, sep=sep)
    return strip_r(s, sep=sep)


def strip_l(s, sep=' '):
    seps=[]
    seps.extend(sep)

    sz=len(s)
    pos=0
    while True:
        if pos < sz:
            for sp in seps:
                if s[pos:pos+1] == sp:
                    pos += 1
                    break
            else:
                break
        else:
            break

    return s[pos:]


def strip_r(s, sep=' '):
    seps=[]
    seps.extend(sep)

    pos=len(s)-1
    while True:
        if pos > -1:
            for sp in seps:
                if s[pos:pos+1] == sp:
                    pos -= 1
                    break
            else:
                break
        else:
            break

    return s[0:pos+1]


def lstrip(s, sep='/', pat='', max=1):
    if len(pat):
        if pat == '##':
            max=1000

    count = 1
    while True:
        pos=s.find(sep)
        if pos == -1:
            break
        else:
            s = s[pos+len(sep):]

        if count == max:
            break

        count += 1

    return s


def rstrip(s, sep=' ', pat='', max=1):
    if len(pat):
        if pat == '%%':
            max=1000

    count = 1
    while True:
        pos=s.rfind(sep)
        if pos == -1:
            break
        else:
            s = s[0:pos]

        if count == max:
            break

        count += 1

    return s

def which(items):
    # items: list or str
    if type(items) == StringType:
        items=[ items ]

    # note that the return value is strictly valid only for a single item
    b=False

    with open(os.devnull, 'w') as DEV_NULL:
        for item in items:
            if subprocess.call(["which", item]
                            , stdout=DEV_NULL, stderr=DEV_NULL ) == 0:
                b=True
            else:
                print "which: no " + item + "in PATH"

    return b
