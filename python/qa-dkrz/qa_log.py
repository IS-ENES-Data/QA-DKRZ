'''
Created on 21.03.2016

@author: hdh
'''

import sys
import os
from   types import *
import qa_util

class Log(object):
    '''
    classdocs
    '''

    def __init__(self, opts):
        '''
        Constructor
        '''

        self.opts = opts

        self.entry = []

        self.entry_id = []
        self.entry_lock = ''
        self.write_lock = ''
        self.line_wrap_sz = 80
        self.word_line_sz = 50

        self.indent = [''] # indentations for yaml
        for i in range(9):
            self.indent.append(self.indent[i] + ' ')


    def append(self, entry_id='',
                f='', d_path=''  , r_path     =''   ,
                start      =[]   , info       =[]   , txt  ='',
                status     =-1   , set_qa_lock = False,
                period=[]        , conclusion =''   , indent=-1,
                is_events  =False, annotation    =''   , impact='', tag='',
                qa_res     =''   , sub_path   =''):

        # this is not self.entry
        entry = []

        if entry_id == '':
            entry.append(self.indent[1] + '- date: ' + qa_util.date())

        if len(f):
            if f[-3:] == '.nc' or f[-4:] == '.nc5':
                s0='file: '
            else:
                s0='data-set: '

            entry.append(self.indent[3] + s0 + f)

        if len(d_path):
            entry.append(self.indent[3] + 'data_path: ' + d_path)

        if len(r_path):
            entry.append(self.indent[3] + 'result_path: ' + r_path)

        if len(period):
            entry.append(self.indent[3] + 'period: ')
            entry.append(self.indent[4] + 'begin: ' + period[0])
            entry.append(self.indent[4] + 'end: ' + period[1])

        if len(conclusion):
            entry.append(self.indent[3] + 'conclusion: ' + conclusion)

        if is_events:
            entry.append(self.indent[3] + 'events:')

        if len(annotation):
            entry.append(self.indent[4] + '- event:')
            entry.append(self.indent[7] + 'annotation: ' + annotation)
            if len(impact):
                entry.append(self.indent[7] + 'impact: ' + impact)
            if len(tag):
                entry.append(self.indent[7] + 'tag: ' + tag)

        if len(info):
            entry.append(self.indent[7] + 'info:')

            for line in info:
                if len(line) > self.line_wrap_sz:
                    # at first test for a line wrap of lines too long
                    lines = self.line_wrap(line)
                else:
                    lines = [ line ]

                for l in range(len(lines)):
                    entry.append(self.indent[8] + '- ' + lines[l])

        if status > -1:
            entry.append(self.indent[3] + 'status: ' + repr(status))

            if set_qa_lock:
                ff = f
                if ff[-3:] == '.nc':
                    ff = ff[:-3]
                elif ff[-4:] == '.nc4':
                    ff = ff[:-4]

                out = os.path.join(qa_res, 'data', sub_path, \
                    'qa_lock_' + ff + '.txt')

                with open(out, 'w') as f_qa_lock:
                    f_qa_lock.write('Path: ' + d_path + '\n')
                    f_qa_lock.write('File: ' + f + '\n')

                    if len(impact) > 0 and len(tag) > 0:
                        f_qa_lock.write(impact + '-' + tag )
                    elif len(impact):
                        f_qa_lock.write(impact)
                    elif len(tag):
                        f_qa_lock.write(tag)

                    f_qa_lock.write(': ' + annotation + '\n')



        (ix, entry_id) = self.get_entry_slot_ix(entry_id, entry)

        self.entry[ix].extend(entry)

        return entry_id


    def check_for_skipping(self, blk, skip_fBase):
        for line in blk:
            x_line = line.split()
            if x_line[0] == 'file:' or x_line[0] == 'data-set:':
                for f in skip_fBase:
                    if f+'.nc' == x_line[1] or f+'.nc4' == x_line[1] or f == x_line[1]:
                        blk = []
                        return

                break

        return


    def get_entry(self, eid):
        entry=[]

        if eid in entry:
            ix = entry.index(eid)
            self.fds.seek( self.block_beg[ix] )

            for line in self.fds:
                if self.fds.tell() < self.block_end[ix]:
                    entry.append(line)
                else:
                    break

        return entry


    def get_entry_slot_ix(self, entry_id, entry):
        # id of a previously found self.entry slot
        if entry_id != '':
            for d in self.entry_id:
                if d['id'] == entry_id:
                    return (d['ix'], entry_id)

        # find something new or released
        entry_id = qa_util.get_md5sum(entry)

        while True:
            if self.entry_lock == '':
                self.entry_lock = entry_id

            if self.entry_lock != entry_id:
                # locked by another process
                continue

            # find a free entry list item
            for ix in range(len(self.entry)):
                if len(self.entry[ix]) == 0:
                    break
            else:
                self.entry.append([])
                ix = len(self.entry)-1

            # find a free entry_id list item
            for ei_ix in range(len(self.entry_id)):
                if self.entry_id[ei_ix]['id'] == '':
                    self.entry_id[ei_ix]['id'] = entry_id
                    self.entry_id[ei_ix]['ix'] = ix
                    break
            else:
                self.entry_id.append( {'id': entry_id, 'ix': ix} )

            # unlock
            self.entry_lock = ''

            break

        return (ix, entry_id)


    def get_next_blk(self, f_log='', get_prmbl=False, skip_fBase=[], fd=''):
        # read the next entry from a log-file or alternatively from provided fd

        if type(fd) == StringType:
            is_fd_explicit=False

            # for having multiple log-files open
            try:
                self.dict_fd
            except:
                self.dict_fd = {}

            if f_log in self.dict_fd.keys():
                # a file is already open
                fd = self.dict_fd[f_log]
            elif len(fd):
                # new
                try:
                    fd = open(f_log)
                    self.dict_fd[f_log] = fd
                except:
                    print 'could not open file ' + f_log
                    return []  # False
        else:
            is_fd_explicit=True

        blk = []
        if len(skip_fBase):
            is_skip_fBase=True
        else:
            is_skip_fBase=False

        # read preamble when the file was just opened
        if fd.tell() == 0:
            self.blk_last_line=''

            # skip the preamble
            for line in fd:
                blk.append(line)

                if 'items:' in line:
                    self.blk_last_line = fd.next()
                    break

            if get_prmbl:
                return blk
            else:
                blk = []

        if len(self.blk_last_line):
            blk.append(self.blk_last_line)

        for line in fd:
            line = qa_util.rstrip(line, sep='\n')
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

        # finalise after EOF.
        # If blk length > 0, then check the current blk,
        # which wasn't done in the loop

        # Another call would have blk = [].
        if is_skip_fBase:
            self.check_for_skipping(blk, skip_fBase)

        if is_fd_explicit:
            self.dict_fd[f_log].close()
            del self.dict_fd[f_log]

        return blk


    def get_TR_find_freq(self, file, g_vars):

        return


    def is_resumed(self):
        if len(self.block_beg):
            return True

        return False


    def line_wrap(self, line, lw=0):
        # Wrap long lines, but preserve words (if reasonable).
        lines=[]
        max_ext = 20  # maximum extension of line wrap
        if lw == 0:
            lw=self.line_wrap_sz

        # split line at blanks, \n and ';'
        words=[]
        w0 = line.split(';')
        for w in w0:
            w1 = w.split('\n')
            for w in w1:
                w2 = w.split()
                for w in w2:
                    words.append(w)

        wline=''
        for word in words:
            word_sz = len(word)
            wline_sz = len(wline)
            if wline_sz:
                wline += ' '

            if (wline_sz + word_sz) < lw:
                wline += word
            elif (wline_sz + word_sz - max_ext) < lw:
                wline += word
                lines.append(wline.rstrip())
                wline=''
            else:
                lines.append(wline.rstrip())
                wline = word
        else:
            if len(wline):
                wline = wline.rstrip()
                if wline[-1] != '.':
                    wline += '.'

                lines.append(wline)

        return lines


    def set_preamble(self, opts):

        entry = ['---']
        entry.append('# Log-file created by QA-DKRZ')
        entry.append('configuration:')

        s = self.indent[1] + 'command-line: '
        for arg in sys.argv:
            s += ' ' + arg
        entry.append(s)

        entry.append(self.indent[1] + 'options:')

        for kv in qa_util.get_sorted_options(opts, sep=': '):
            if kv[:7] == 'TABLES:':
                entry.append(self.indent[2] + 'TABLES:')
                for tkv in qa_util.get_sorted_options(opts['TABLES'], sep=': '):
                    entry.append(self.indent[3] + '- ' + tkv)
                continue

            elif kv[:6] == 'TABLE_':
                # taken into account above
                continue

            elif kv[:8] == 'install:':
                # skip internal opt
                continue

            entry.append(self.indent[2] + kv)


        entry.append('start:')
        entry.append(self.indent[1] + 'date: ' + qa_util.date())
        entry.append(self.indent[1] + 'qa-revision: ' + self.opts['QA_VERSION'])
        entry.append('items:')

        return entry


    def summary(self, f_log, g_vars):
        # extraction of annotations and atomic time ranges from log-files

        # sub-directories in check_logs
        self.f_annot = os.path.join(g_vars.check_logs_path, 'Annotations')
        self.f_perd  = os.path.join(g_vars.check_logs_path, 'Period')
        self.f_sum   = os.path.join(g_vars.check_logs_path, 'Summary')

        qa_util.mkdirP(self.f_annot)
        qa_util.mkdirP(self.f_perd)
        qa_util.mkdirP(self.f_sum)

        # time range of atomic variables; in order to save mem,
        # beg and end, respectively, are linked to the name by the
        # index of the corresponding atomic variable name in prd_name
        self.prd_name=[]
        self.prd_beg={}
        self.prd_end={}

        # reading and processing of the logfile
        with open(f_log, 'r') as fd:
            while True:
                blk = self.get_next_blk(fd=fd)

                if len(blk) == 0:
                    break

                for i in range(len(blk)):
                    words = blk[i].split()

                    if words[0] == 'file:':
                        fName = words[1]
                    elif words[0] == 'period:':
                        # time ranges of atomic variables
                        self.update_period(fName, blk, i)
                        continue

        return


    def update_period(self, fName, blk, i):
        # t_r contains ( fBase, StartTime, EndTime )
        t_r = qa_util.f_time_range(fName)

        try:
            ix = self.prd_name.index(t_r[0])
        except:
            ix = len(self.prd_name)
            self.prd_name.append(t_r[0])
            self.prd_beg[ix] = 'x'  # greater than any date
            self.prd_end[ix] = ''   # smaller than any date

        words = blk[i+1].split()
        if words[0] == 'begin:' and words[1] < self.prd_beg[ix]:
            self.prd_beg[ix] = words[1]

        words = blk[i+2].split
        if words[0] == 'end:' and words[1] > self.prd_end[ix] :
            self.prd_end[ix] = words[1]

        return


    def write_entry(self, entry_id, log_dir, log_fname):

        while True:
            if self.write_lock == '':
                self.write_lock = entry_id

            if self.write_lock == entry_id:
                for d in self.entry_id:
                    if d['id'] == entry_id:
                        ix = d['ix']
                        break
                else:
                    # this should not happen
                    print 'Log.write_entry(): unknown entry_id'

                # a logfile is available

                log_file = os.path.join(log_dir, 'tmp_' + log_fname + '.log')

                # decider whether a preamble has to be written
                isOldFile = os.path.isfile(log_file)
                #if not self.isResumedSession:
                #    old_log_file = os.path.join(log_dir, log_fname + '.log')
                #    self.isResumedSession = os.path.isfile(old_log_file)

                with open(log_file, 'a') as fd:

                    # write the preamble
                    if not isOldFile:
                        entry = self.set_preamble(self.opts)
                        for e in entry:
                            fd.write(e + '\n')

                    for e in self.entry[ix]:
                        fd.write(e + '\n')

                self.write_lock = ''
                break

        return
