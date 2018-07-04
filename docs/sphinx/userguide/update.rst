.. _update:

======
Update
======

Updates of tables are downloaded from external sources by

.. code-block:: bash

   $ qa-dkrz install [opts] PROJECT-Name

with options:

PROJECT-name (as single parameter)
    installation of external tables for the named project. Additionally,
    compilation of executables for PROJECT, but only for GitHub based installation.

\- -force [PROJECT]
    Unconditional update. Also required to unlock a frozen state.

\- -freeze
    Lock current installation; `HOME/.qa-dkrz/config.txt` is copied to the root  directory of QA-DKRZ. This is applied by every user of this particular
    installation and can not be modified.

\- -up  [PROJECT]
    Updating tables only when there was no update on that day before. The frequency may be prolonged by option ``--uf=num`` with ``num`` in days.

\- -ship
    Prepare a working installation for transferring it to internet-free devices. (in preparation)

\- -shipped
    internet-free installation. (in preparation)

Note that ``qa-dkrz`` acts a wrapper for conda both for python and the bash script.
However when running from the GitHub repository, the direct call is preferable.

