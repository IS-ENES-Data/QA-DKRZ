#!/bin/bash

get_QA_path()
{
   local i items p src
   declare -a items

   p=$0

   while [ -h $p ] ; do
      # resolve symbolic links: cumbersome but robust,
      # because I am not sure that ls -l $p | awk '{print $11}'
      # works for any OS

      items=( $(ls -l $p) )
      i=$((${#items[*]}-1))
      p=${items[i]}
   done

   p=${p%/*}

   # resolve relative path
   if [ ${p:0:1} != '/' ] ; then
     cd $p &> /dev/null
     p=$(pwd)
     cd - &> /dev/null
   fi

   QA_PATH=${p%/*}

   return
}

args=( $* )
if [ "${args[0]%%=*}" = 'install' -o "${args[0]%%=*}" = '--install' ] ; then
  if [ "${args[0]#*=}" != "${args[0]}" ] ; then
     args[0]="${args[0]#*=}"
  else
     args[0]=
  fi
  isInstall=t
fi

get_QA_path

unset LD_LIBRARY_PATH
#export LD_LIBRARY_PATH=${QA_PATH}/lib

if [ ${isInstall:-f} = t ] ; then
  #exec ${QA_PATH}/opt/qa-dkrz/install $*
  exec ${QA_PATH}/opt/qa-dkrz/install ${args[*]}
else
  exec ${QA_PATH%envs/*}/bin/python ${QA_PATH}/opt/qa-dkrz/python/qa-dkrz/qa-dkrz.py $*
fi

