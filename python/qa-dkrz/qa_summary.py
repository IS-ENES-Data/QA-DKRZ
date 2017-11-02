#! /hdh/local/miniconda/bin/python

'''
Created on 21.03.2016

@author: hdh
'''

import copy
import glob
import os
import sys
from types import *

import qa_util

class LogSummary(object):
    '''
    classdocs
    '''

    def __init__(self):
        '''
        Constructor
        '''

        # defaults
        self.project=''
        self.prj_fName_sep = '_'
        self.prj_path_sep = '/'
        self.prj_var_ix = 0
        self.prj_frq_ix = 1


    def annotation_add(self, path_id, var_id, blk, i):
        # annotation and associated indices of properties
        # fse contains ( var, StartTime, EndTime )
        # index i points to blk[i] == 'caption:'

        # just to have them defined in case of an error
        capt=''
        impact=''
        tag=''

        words = blk[i+1].lstrip(' -').split(None,1)
        if words[0] == 'caption:':
            capt = words[1]
            i=i+1

        words = blk[i+1].lstrip(' -').split(None,1)
        if words[0] == 'impact:':
            impact = words[1]
            i=i+1

        words = blk[i+1].lstrip(' -').split(None,1)
        if words[0] == 'tag:':
            tag = words[1]
            j=0
            if tag[0] == "'":
                j=1

            if tag[j:j+2] == 'SF' or tag[j]=='M':
                #  when these kinds of tags appear, there is only a single event
                self.file_count -= 1

            i=i+1

        # the sub-temporal range of a given variable (and path)
        try:
            # index of a found caption
            ix = self.annot_capt.index(capt)
        except:
            # index of a new caption
            ix = len(self.annot_capt)

            self.annot_capt.append(capt)
            self.annot_tag.append(tag)
            self.annot_impact.append(impact)

            self.annot_fName_id.append([var_id])
            self.annot_path_id.append([path_id])

            self.annot_fName_dt_id.append([[self.dt_id]])
        else:
            # a registered annotation

            # associated properties
            try:
                jx = self.annot_path_id[ix].index(path_id)
            except:
                # new path for the current annotation
                # jx = len( self.annot_path_id[ix] )
                self.annot_path_id[ix].append(path_id)

            try:
                jx = self.annot_fName_id[ix].index(var_id)
            except:
                # new variable for the current annotation
                jx = len( self.annot_fName_id[ix] )
                self.annot_fName_id[ix].append(var_id)
                self.annot_fName_dt_id[ix].append([])

            try:
                self.annot_fName_dt_id[ix][jx].index(self.dt_id)
            except:
                # kx = len(self.annot_fName_dt_id[ix][jx])
                self.annot_fName_dt_id[ix][jx].append(self.dt_id)

        j=i

        return j


    def annotation_getItems(self, ix, sep='/'):
        # ix: index of annotation
        # self.prj_var_ix: path with variable as i-th component
        # self.prj_frq_ix: path with frequency as i-th component

        # find identical indices of path and variable items
        # within the annot_..._ids

        # Find the beginning of a path common for the entire annotation.
        # Note that each index of p_items stores a list of corresponding
        # path components.

        # init of pItems[[]] found for this, i.e. ix-th, annotation.
        # Mutable items are represented by '*'
        # pItems takes into account the possibility that different annotation
        # paths are of different size. pItems_api[[]] gives corresponding
        # api elements.
        if sep == '/':
            axi = self.annot_path_id[ix]
        else:
            axi = self.annot_fName_id[ix]

        return self.get_annot_source(axi, sep)


    def annotation_merge(self):
        # find condensed scopes of annotations

        pItems=[]
        pItems_aix=[]
        pMutable=[]

        annot_sz = len(self.annot_capt)

        if annot_sz == 0:
            # not a single annotation
            self.conclusion='PASS'

            # begin of writing the JSON files
            fl = os.path.join(self.f_annot, self.log_name + '.json')

            with open(fl, 'w') as fd:
                # pItems across ix are now all the same
                self.write_no_annot(fd)
            return

        self.conclusion='FAIL'
        p_sz=annot_sz

        # there is not a single annotation
        if annot_sz == 0:
            return

        for ix in range(annot_sz):
            # get scope
            a, b = self.annotation_getItems(ix)
            pItems.append(copy.deepcopy(a))
            pMutable.append(b)

        pItems_com=[] # independent on ix

        sz_min=len(pMutable[0])
        sz_max=len(pMutable[0])
        for jx in range(1, annot_sz):
            sz=len(pItems[jx])
            if sz_min > sz:
                sz_min=sz
            if sz_max < sz:
                sz_max=sz

        # distinguish between static pItems elements and multiples,
        # the latter combine to '*'
        for mx in range(sz_min):
            isAst=False
            for jx in range(1, annot_sz):
                if jx < sz_min:
                    if pItems[jx][mx] != pItems[0][mx]:
                        isAst=True
                else:
                    isAst=True
            else:
                if isAst:
                    pItems_com.append(0)
                else:
                    pItems_com.append(pItems[0][mx])

        # the ragged tail items get '*', because of paths
        # different in number of items
        for mx in range(sz_min, sz_max):
            pItems_com.append(0)

        # replace original pItems
        for ix in range(annot_sz):
            pItems[ix] = pItems_com

        #delete empty items and start with basePath
        self.pItems_eff_ix=[]  # set -1 for delete candidates
        for i in range(len(pItems_com)):
            item=self.p_items[pItems_com[i]]

            if len(item):
                if item == self.pathBase:
                    # found a starting point, adjust beginning
                    for j in range(i):
                        self.pItems_eff_ix[j]=-1
                else:
                    self.pItems_eff_ix.append(i)
            else:
                self.pItems_eff_ix.append(-1)


        # get rid of shallow copy properties of pItems
        annot_sz_ixs = range(annot_sz)
        ppItems=[]
        for ix in annot_sz_ixs:
            ppItems.append( copy.deepcopy(pItems[ix]) )

        # get rid of empty elems or non DRS beginnings
        pItems = ppItems
        while True:
            for j in range(len(self.pItems_eff_ix)):
                if self.pItems_eff_ix[j] == -1:
                    for ix in annot_sz_ixs:
                        del pItems[ix][j]
                    for ix in annot_sz_ixs:
                        del pMutable[ix][j]

                    del self.pItems_eff_ix[j]
                    break
            else:
                break



        # across annotations: combine almost identical ones whith a <sub-string>
        # appearing also in the path

        # ------
        # begin of writing the JSON files
        fl = os.path.join(self.f_annot, self.log_name + '.json')

        with open(fl, 'w') as fd:
            # pItems across ix are now all the same
            self.write_json_header(pItems[0], fd)

            # write annotations for given path prefixes or a kind of MIP name
            isInit=True

            for ix in range(annot_sz):
                self.write_json_annot(ix, fd,
                        pItems=pItems[ix],
                        pMutable=pMutable[ix],
                        init=isInit )

                isInit=False

            self.write_json_annot(0, fd, final_annot=True, final=True )

        '''
        # write a file with annotation, path and file name for each tag
        for ix in range(len(self.annot_tag)):
            # basePath = self.getPathPrefix(uniquePaths[jx], pItems[f0_ix][jx])
            tag = self.annot_tag[ix].strip("'")

            name = self.annot_impact[ix] + '_'+ tag
            fl = os.path.join(self.tag_dir, tag)

            with open(fl, 'w') as fd:
                self.write_tag_file(ix, tag, uniquePaths,
                    pItems[ix],
                    fd)
        '''

        return


    def annotation_period_reset(self, var_id):
        # resets for a particular atomic variable

        for ix in range(len(self.annot_capt)-1,-1,-1):
            try:
                jx = self.annot_fName_id[ix].index(var_id)
            except:
                continue
            else:
                # there is something to clear.
                del self.annot_fName_dt_id[ix][jx]
                #del self.annot_fName_id[ix][jx]
                #del self.annot_path_id[ix][jx]

        return

    def annot_synthetic_tag(self):
        # create a synthetic tag for annotations that have none;
        # this will not eventually be printed.
        annot_sz = len(self.annot_tag)

        # there is not a single annotation
        if annot_sz == 0:
            return

        for ix in range(annot_sz):
            if len(self.annot_tag[ix]) == 0:
                md5 = qa_util.get_md5sum(self.annot_capt[ix])

                # try for 3 digits
                width=3
                while True:
                    if md5[0:width] in self.annot_tag:
                       width=4
                    else:
                       self.annot_tag[ix]=md5[0:width]
                       break

        return


    def check_for_skipping(self, blk, skip_fBase):
        for line in blk:
            x_line = line.split()
            if x_line[0] == 'file:':
                for f in skip_fBase:
                    if f+'.nc' == x_line[1] or f == x_line[1]:
                        blk = []
                        return

                break

        return


    def decomposition(self, name, items, lst_ids, sep):
        # Decompose a filename or a path.
        if name[-3:] == '.nc':
            name=name[0:-3]

        itm_id=[]

        x_name = name.split(sep)
        sz = len(x_name)

        # a variable special: discard trailing date-range
        dtr = self.dt[self.dt_id]
        if dtr == x_name[sz-1]:
            sz -= 1

        # members of a the DRS components
        if sep == self.prj_path_sep:
            # init
            if len(self.p_drs) == 0:
                for i in range(sz):
                    self.p_drs.append([])

            # update
            for i in range(sz):
                if not x_name[i] in self.p_drs[i]:
                    self.p_drs[i].append(x_name[i])

        for i in range(sz):
            # update the list of var or path items, respectively
            try:
                itm_id.append( items.index(x_name[i]) )
            except:
                itm_id.append(len(items))
                items.append(x_name[i])

        try:
            ID = lst_ids.index(itm_id)
        except:
            ID = len(lst_ids)
            lst_ids.append(itm_id)

        return ID


    def get_annot_source(self, a_item_ix, sep='/'):
        # a_item_ix: annotation path indices

        # references
        if sep == '/':
            # basic path
            ref_ids = self.path_ids
            ref_items = self.p_items
        else:
            # basic filename
            ref_ids = self.fName_ids
            ref_items = self.f_items

        mutables=[]

        for i in range(len(a_item_ix)):
            aix=a_item_ix[i]
            itm=ref_items[aix]

            if len(itm):
                for j in range(len(ref_ids[aix])):
                    k = copy.deepcopy(ref_ids[aix][j])

                    try:
                        mutables[j]
                    except:
                        mutables.append([k])
                    else:
                        for m in range(1,len(mutables[j])):
                            if mutables[j][m] != mutables[j][0]:
                                mutables[j].append(k)


        #if sep == '/':
        items=[]
        for m in range(len(mutables)):
            if len(mutables[m]) == 1:
                items.append(mutables[m][0])
            else:
                items.append(0)  # means self.p_items[0] --> '*'

        return items, mutables


    def get_next_blk(self, fd, f_log='', get_prmbl=False, skip_fBase=[]):
        # read the next entry from a log-file or alternatively from provided fd

        blk = []
        if len(skip_fBase):
            is_skip_fBase=True
        else:
            is_skip_fBase=False

        # read preamble when the file was just opened
        if fd.tell() == 0:
            self.blk_last_line=''

            # items from the preamble
            for line in fd:
               line = line.rstrip(' \n')
               blk.append(line)

               words = line.split(None, 1)

               if words[0] == 'items:':
                  self.blk_last_line = fd.next().rstrip(' \n')
                  break

               # a few opts are used by this script
               self.get_preamble_opt(words)
            else:
               self.get_preamble_opt(words, final=True)


            # for logPathIndex, pathBase has also to be defined
            try:
                self.logPathIndex
            except:
                pass
            else:
                try:
                    self.pathBase
                except:
                    del self.logPathIndex

            if get_prmbl:
                return blk
            else:
                blk = []

        if len(self.blk_last_line):
            blk.append(self.blk_last_line)

        for line in fd:
            line = line.rstrip(' \n')
            if len(line) == 0:
                continue

            if '- date:' in line:
                # eventually for the current block
                if len(blk):
                    # blk is reset for skip condition True
                    if is_skip_fBase:
                        self.check_for_skipping(blk, skip_fBase)

                    # Preserve the current line for next blk.
                    self.blk_last_line = line
                    return blk

            blk.append(line)
        else:
            # the last entry was just read
            self.blk_last_line = ''

        # finalise after EOF.
        # If blk length > 0, then check the current blk,
        # which wasn't done in the loop

        # Another call would have blk = [].
        if is_skip_fBase:
            self.check_for_skipping(blk, skip_fBase)

        return blk


    def getPathPrefix(self, path): #, astItems):
        # Default: turn a path into '_' separated string.
        # If given, use a project provided identification string instead.
        # If available, use LOG_PATH_INDEX for assembling a unique name

#        path = prefix
#        path.extend(astItems)
#        if len(path[0]) == 0:
#            del path[0]

        s=''
        sz = len(path)

        try:
            currIx = self.logPathIndex
        except:
            currIx = range(len(path))

        for i in range(len(currIx)):
            ix = int(currIx[i])
            if ix < sz and path[ix] != '*':
                if len(s) > 0:
                    s += '_'
                s += path[ix]

        return s


    def period_add(self, var_id, path_id, fse, blk, i):
        # Requires old-to-young sorted sub-temporal files of an atomic variable
        # the procedure takes a missing 'begin' but present 'end' into account;
        # also missing of the two what should never happen.

        beg = blk[i+1].split()
        if beg[0] == 'begin:':
            i=i+1
            if len(self.atomicBeg[var_id]) == 0:
                self.atomicBeg[var_id] = beg[1]

        end = blk[i+1].split()
        if end[0] == 'end:':
            i=i+1
            if end[1] > self.atomicEnd[var_id] :
                self.atomicEnd[var_id] = end[1]
            else:

                # suspecting a reset of an atomic variable
                self.annotation_period_reset(var_id)

                self.atomicBeg[var_id] = beg[1]
                self.atomicEnd[var_id] = end[1]

        return i


    def period_final(self):
        # find indices for the different frequencies,
        # note that these are dependent on the project

        frqs=[]
        frqs_id=[] # this will be expanded to a 2D array
        frqs_beg=[] # this will be expanded to a 2D array
        frqs_end=[] # this will be expanded to a 2D array
        sz_max=0

        for i in range(len(self.fName_ids)-1):
            #if self.project == 'CORDEX':
            name = self.reassemble_name(self.f_items,
                                        self.fName_ids[i],
                                        self.prj_fName_sep)
            sz = len(name)
            if sz > sz_max:
                sz_max = sz

            word = name.rsplit('_', 2)
            for j in range(len(frqs)):
                if word[2] == frqs[j]:
                    frqs_id[j].append(i)
                    break
            else:
                # append new lists
                frqs.append(word[2])
                frqs_id.append([i])
                frqs_beg.append(self.atomicBeg[i])
                frqs_end.append(self.atomicEnd[i])

        # find the most extended begin and end, respectively, for each frequency
        for j in range(len(frqs)):
            ix = frqs_id[j]
            for i in range(1, len(ix)):
                if frqs_beg[j] > self.atomicBeg[ix[i]]:
                    frqs_beg[j] = self.atomicBeg[ix[i]]

                if frqs_end[j] < self.atomicEnd[ix[i]]:
                    frqs_end[j] = self.atomicEnd[ix[i]]

        # yaml output to a file in directory 'Period'
        f_period  = os.path.join(self.f_period, self.log_name + '.period')

        with open(f_period, 'w') as fd:
            fd.write("--- # Time intervals of atomic variables.\n")

            for j in range(len(frqs)):
                line =  '\n- frequency: ' + frqs[j] + '\n'
                line += '  number_of_variables: ' + repr(len(frqs_id[j]))
                fd.write(line + '\n')

                for k in range(len(frqs_id[j])):
                    ix = frqs_id[j][k]

                    line  = '  - variable: '
                    line += self.reassemble_name(self.f_items,
                                                      self.fName_ids[ix],
                                                      self.prj_fName_sep) + '\n'

                    if self.atomicBeg[ix] == '':
                        line += '    begin:\n'
                    else:
                        line += '    begin: ' + self.atomicBeg[ix] + '\n'

                    line += '    end: ' + self.atomicEnd[ix] + '\n'

                    status = ''
                    if frqs_beg[j] !=  self.atomicBeg[ix]:
                        status += 'B\n'
                    if frqs_end[j] !=  self.atomicEnd[ix]:
                        status += 'E\n'

                    if len(status):
                        line += '    status: FAIL:' + status
                    else:
                        line += '    status: PASS\n'

                    fd.write(line)

        # human-readable output into Summary
        hyphen     = ' - '
        mark_ok    = '    '
        mark_left  = '--> '
        mark_right = ' <--'
        sz_max += 1

        self.f_range  = os.path.join(self.f_period, self.log_name + '.range')

        with open(self.f_range, 'w') as fd:
            fd.write("# Time intervals of atomic variables.\n")

            for j in range(len(frqs)):
                line =  '\nFrequency: ' + frqs[j] + ','
                line += ' number of variables: ' + repr(len(frqs_id[j]))
                fd.write(line + '\n')

                for k in range(len(frqs_id[j])):
                    ix = frqs_id[j][k]

                    line  = self.reassemble_name(self.f_items,
                                                 self.fName_ids[ix],
                                                 self.prj_fName_sep).ljust(sz_max)

                    if frqs_beg[j] !=  self.atomicBeg[ix]:
                        line += mark_left
                    else:
                        line += mark_ok

                    line += self.atomicBeg[ix] + hyphen
                    line += self.atomicEnd[ix]

                    if frqs_end[j] !=  self.atomicEnd[ix]:
                        line += mark_right
                    else:
                        line += mark_ok

                    fd.write(line + '\n')

        return


    def get_preamble_opt(self, words, final=False):
         if final:
             if self.pathBase:
                pos=self.prj_data.find('/'+self.pathBase+'/')
                if pos > -1:
                    self.prj_data=self.prj_data[pos+1:]

             for i in range(len(self.spl)):
                self.spl[i] = self.prj_data + '/' + self.spl[i]

             return

         if words[0] == 'PROJECT:' and len(self.project) == 0:
            self.project = words[1]
         elif words[0] == 'FILE_NAME_SEP':
            self.prj_fName_sep = words[1]
         elif words[0] == 'PATH_SEP':
            self.prj_path_sep = words[1]
         elif words[0] == 'FILE_NAME_VAR_INDEX':
            self.prj_var_ix = words[1]
         elif words[0] == 'FILE_NAME_FREQ_INDEX':
            self.prj_frq_ix = words[1]
         elif words[0] == 'DRS_PATH_BASE:' \
               or words[0] == 'LOG_PATH_BASE:' \
                     or words[0] == 'EXP_PATH_BASE:':
            self.pathBase = words[1]
         elif words[0] == 'LOG_PATH_INDEX:' \
               or words[0] == 'EXP_PATH_INDEX:':
            # the following does not seem to work, although it should
            # self.logPathIndex = words[1].split('[, ]')
            #so, a work-around
            w2=words[1].replace('[','')
            w2=w2.replace(']','')
            self.logPathIndex = w2.split(', ')
         elif words[0] == 'SELECT_PATH_LIST:':
            lst=words[1].split(',')
            for l in lst:
                try:
                    self.spl.append(qa_util.strip(l," []'"))
                except:
                    self.spl=[qa_util.strip(l," []'")]
         elif words[0][0:12] == 'PROJECT_DATA':
            try:
                self.prj_data.append(words[1])
            except:
                self.prj_data=[words[1]]

         return

    def prelude(self, f_log):
        if type(f_log) == StringType:
            f_log = [f_log]

        # 1) path only applies to each file without a path
        # 2) file with a path as is.
        # 3) first occurrence of a path either from file or alone
        #    applies to 1)

        coll=[]
        paths=[]
        path=''

        for i in range( len(f_log) ):
            if os.path.isdir(f_log[i]):
               if len(path) == 0:
                  path = f_log[i]
            else:
                if f_log[i][-4:] != '.log':
                    f_log[i] = f_log[i] + '.log'

                # look for a path component
                head, tail = os.path.split(f_log[i])

                if len(path) == 0 and len(head) > 0:
                   path = head

                if len(head) == 0:
                    paths.append(path)
                else:
                    paths.append(head)

                coll.append(tail)

        if len(coll) == 0:
            coll = glob.glob(path + '/*.log')

        # could be empty
        f_log=[]
        for i in range(len(coll)):
            head, tail = os.path.split(coll[i])

            if len(paths) and len(paths[i]):
               p=paths[i]
            else:
               p=path

            f_log.append( os.path.join(p, tail))

        return f_log  # always a list


    def reassemble_name(self, items, lst_ix, sep):
        # Assemble the name from the components, collected in lst_ix (path ar var)
        # Note that for a path the leading '/' is represented by an empty list item,
        # which is here reassembled neatly.

        sz = len(lst_ix)
        name=''

        if sz:
            name = items[lst_ix[0]]

            for i in range(1, sz):
                name += sep + items[lst_ix[i]]

        return name


    def run(self, f_log):
        if not os.path.isfile(f_log):
            print 'qa_summary: ' + f_log + ' : no such file'
            return

        # extraction of annotations and atomic time ranges from log-files
        self.log_path, self.log_name = os.path.split(f_log)
        self.log_name = self.log_name[0:-4]

        # sub-directories in check_logs
        self.f_annot = os.path.join(self.log_path, 'Annotations')
        self.f_period  = os.path.join(self.log_path, 'Period')
        self.tag_dir = os.path.join(self.log_path, 'Tags', self.log_name)

        qa_util.mkdirP(self.f_annot)
        qa_util.mkdirP(self.f_period)
        #qa_util.mkdirP(self.tag_dir)

        # time range of atomic variables; in order to save mem,
        # beg and end, respectively, are linked to the name by the
        # index of the corresponding atomic variable name in var
        self.fName_ids=[]
        self.fName_dt_id={}  # each fName_id gets a list of dt ids
        self.path_ids=[]
        self.f_items=[]
        self.p_items=['*']  # a placeholder
        self.p_drs=[]

        self.var_ids={}  # contains all [ids] with the same variable name in {}

        self.atomicBeg=[]         # atomic time interval
        self.atomicEnd=[]         # atomic time interval
        self.dt=[]                # time intervals of sub-temp files

        self.annot_capt=[]        # brief annotations
        self.annot_impact=[]      # corresponding  severity level
        self.annot_tag=[]         # corresponding tag
        #self.annot_scope=[]       # brief annotations
        self.annot_fName_id=[]    # for each var involved
        self.annot_path_id=[]
        # self.annot_var_ix=[]      # only project variable names
        self.annot_fName_dt_id=[] # for each time interval of each var involved

        # count total occurrences (good and bad)
        self.file_count=0
        self.var_dt_count=[]  # for all frequencies

        # reading and processing of the logfile
        if not os.path.isfile(f_log):
            return

        with open(f_log, 'r') as fd:
            while True:
                # read the lines of the next check
                blk = self.get_next_blk(fd=fd)
                sz = len(blk) - 1
                if sz == -1:
                    break

                isMissPeriod=True  # fx or segmentation fault

                line_num=-1
                while line_num < sz :
                    line_num = line_num+1
                    words = blk[line_num].lstrip(' -').split(None,1)
                    if len(words) == 0:
                        # a string of just '-' would results in this
                        words=['-----------']

                    if words[0] == 'file:':
                        # fse contains ( var, StartTime, EndTime ); the
                        # times could be empty strings or EndTime could be empty
                        fse = qa_util.f_time_range(words[1])
                        self.set_curr_dt(fse)
                        fName_id_ix = self.decomposition(words[1], self.f_items,
                                                    self.fName_ids, self.prj_fName_sep)
                        self.file_count += 1

                        # for counting atomic variable's sub_temps for all freqs
                        try:
                            self.fName_dt_id[fName_id_ix]
                        except:
                            self.fName_dt_id[fName_id_ix] = [self.dt_id]
                        else:
                            self.fName_dt_id[fName_id_ix].append(self.dt_id)

                        try:
                            vName = self.f_items[self.fName_ids[fName_id_ix][self.prj_var_ix]]
                        except:
                            pass
                        else:
                            # dict of varNames to contained var_ids
                            try:
                                self.var_ids[vName]
                            except:
                                self.var_ids[vName] = [fName_id_ix]
                            else:
                                try:
                                    self.var_ids[vName].index(fName_id_ix)
                                except:
                                    self.var_ids[vName].append(fName_id_ix)


                        if fName_id_ix > len(self.atomicBeg) - 1 :
                            # init for a new variable
                            self.atomicBeg.append('')  # greater than any date
                            self.atomicEnd.append('')   # smaller than any date

                    elif words[0] == 'data_path:':
                        # used later
                        path_id = self.decomposition(words[1], self.p_items,
                                                     self.path_ids, self.prj_path_sep)

                    elif words[0] == 'period:':
                        # time ranges of atomic variables
                        # indexed by self.curr_dt within the function
                        line_num = self.period_add(fName_id_ix, path_id, fse, blk, line_num)
                        isMissPeriod=False

                    elif words[0] == 'event:':
                        if isMissPeriod and len(fse[2]):
                            self.subst_period(fName_id_ix, path_id, fse)
                            isMissPeriod=False

                        # annotation and associated indices of properties
                        line_num = self.annotation_add(path_id, fName_id_ix, blk, line_num)

                    elif words[0] == 'status:':
                        if isMissPeriod and len(fse[2]):
                            self.subst_period(fName_id_ix, path_id, fse)

        if self.file_count == 0:
            return

        # test for ragged time intervals of atomic variables for given frequency
        self.period_final()

        self.annot_synthetic_tag()

        self.annotation_merge()

        self.sendMail()

        return


    def sendMail(self):
      return


    def set_curr_dt(self, fse):
        dt = ''
        if len(fse[1]):
            dt = fse[1]
            if len(fse[2]):
                dt += '-' + fse[2]

        try:
            self.dt_id = self.dt.index(dt)
        except:
            self.dt_id = len(self.dt)
            self.dt.append(dt)

        return


    def subst_period(self, var_id, path_id, fse):
        # convert DRS date to real date
        tmplt = '0000-01-01T00:00:00'
        dt = [ tmplt, tmplt ]

        for i in range(2):
            j=i+1

            dt[i] = fse[j][0:4].rjust(4,'0') + dt[i][4:]
            if len(fse[1]) > 4:
                dt[i] = dt[i][0:5] + fse[j][4:6].rjust(2,'0') + dt[i][7:]

                if len(fse[j]) > 6:
                    dt[i] = dt[i][0:8] + fse[j][6:8].rjust(2,'0') + dt[i][10:]

                    if len(fse[j]) > 8:
                        dt[i] = dt[i][0:11] + fse[j][8:10].rjust(2,'0') + dt[i][13:]

                        if len(fse[j]) > 10:
                            dt[i] = dt[i][0:14] + fse[j][10:12].rjust(2,'0') + dt[i][16:]

                            if len(fse[1]) > 12:
                                dt[i] = dt[i][0:17] + fse[j][12:4].rjust(2,'0')


        t_blk=['period:']
        t_blk.append('begin: ' + dt[0])
        t_blk.append('end: ' + dt[1])

        fse = ( fse[0], dt[0], dt[1] )

        self.period_add(var_id, path_id, fse, t_blk, 0)

        return


    def write_json_header(self, path, fd):
        # write header of QA results

        tab='    '

        fd.write('{\n')
        fd.write(tab + '"QA_conclusion": "' + self.conclusion + '",\n')
        fd.write(tab + '"project": "' +  self.project + '",\n')

        self.mutable_DRS=[]

        for i in range(len(path)):
            s='"DRS_' + str(i)
            fd.write(tab + s + '": "')

            if path[i] == 0:  # i.e. '*'
                self.mutable_DRS.append(s)
                fd.write('mutable",\n')
            else:
                fd.write(self.p_items[path[i]] + '",\n')

        return


    def write_json_annot(self, ix, fd,
                            pItems=[], pMutable=[],
                            init=False, final=False, final_annot=False):
        # write annotation
        tab='    '

        keys=[]

        if final_annot:
            fd.write(tab + '\n' + tab +']\n')
            if final:
                fd.write('}\n')

            return

        if final:
            fd.write('}\n')
            return

        if init:
            fd.write(tab + '"annotation":\n')
            fd.write(tab + '[\n')
            fd.write(2*tab + '{\n')
        else:
            fd.write(',\n' + 2*tab + '{\n')

        mutable=[]
        for p in range(len(pItems)):
            if not pItems[p]:  # 0 --> '*'
                mutable.append(pMutable[p])

        for k in range(len(self.mutable_DRS)):
            fd.write(3*tab + self.mutable_DRS[k] + '": ')

            keys=''
            for m in mutable[k]:
                p_item=self.p_items[m]

                if len(keys):
                    keys +=', '
                keys += '"' + p_item + '"'

            fd.write(keys + ',\n')

        capt = self.annot_capt[ix].strip("'")
        fd.write(3*tab + '"caption": "' + capt )

        if len(self.annot_impact[ix]):
            impact = self.annot_impact[ix].strip("'")
            fd.write(",\n" + 3*tab + '"severity": "' + impact + '"\n')
        else:
            fd.write("\n")

        fd.write(2*tab + '}')

        return


    def write_no_annot(self, fd):
        # write header of QA results

        self.mutable_DRS=[]

        tab='    '

        fd.write('{\n')
        fd.write(tab + '"QA_conclusion": "' + self.conclusion + '",\n')
        fd.write(tab + '"project": "' +  self.project + '",\n')

        paths=[]

        for spl in self.spl:
            paths.append(spl.split('/'))

        sz=len(paths[0])
        for i in range(1,len(paths)):
            sz2=len(paths[i])
            if sz2 < sz:
                for j in range(sz2,sz):
                    paths[i].append('*')

        items=[]
        for ip in range(sz):
            items.append([])

            for i in range(len(paths)):
                if ip < len(paths[i]):
                    if not paths[i][ip] in items[ip]:
                        items[ip].append(paths[i][ip])


        sz0=len(items)

        for i in range(sz0):
            sz=len(items[i])
            if sz:
                s = tab + '"DRS_' + str(i) + '": "'

                if sz == 1:
                    s += '"' + items[i][0] +'"'
                else:
                    # component is multiply set
                    for m in range(len(items[i])):
                        if m == 0:
                            s += '[ '
                        else:
                            s += ', '

                        s += '"' + items[i][m] + '"'

                    s += ' ]'

                if i < sz0-1:
                    s += ','
                s += '\n'

                fd.write(s)


        fd.write('}')

        return


    def write_tag_file(self, ix,  tag, uniquePaths,
                            pItems,
                            fd):
        # write annotation files for tag[ix]

        capt = self.annot_capt[ix].strip("'")
        impact = self.annot_impact[ix].strip("'")
        #tag = self.annot_tag[ix].strip("'")

        fd.write('annotation: ' + capt + '\n')

        if len(self.annot_impact[ix]):
            fd.write('impact:     ' + impact + '\n')

        fd.write('tag:        ' + tag + '\n')

        return


if __name__ == '__main__':
    summary = LogSummary()
    f_logs = summary.prelude(sys.argv[1:])

    for f_log in f_logs:
        summary.run(f_log) # prelude returns always a list
