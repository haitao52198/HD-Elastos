#!/usr/bin/env python
#
# Sync the source code of seL4test from github.
#
import os
import re
import mock
import tempfile
import json, git

info     = []    # information of repo to sync
temproot = ""    # temp directory for sync
tempgit  = None  # a Git object of temproot
root     = ""    # root path of project
project  = None  # Git for the whole project

def Sync():
    """
    Sync all repo in project.
    """
    for i in range(len(info)):
        repo = info[i]
        local_path = repo["local"]
        remote = repo["remote"]
        old_date = repo["date"]
        cur_date = SyncSimple(local_path, remote, old_date)
        info[i]["date"] = cur_date
        print "Sync %s [%s]" % (local_path, cur_date)
    print "Sync Finished!"
    
    # Update version.json
    json.dump(info, open('version.json', 'w'))
    project.add('sync')
    project.commit('-m', '"Update[sync/version.json]"')
    # project.push()

def SyncSimple(local_path, remote, old_date):
    """
    Sync a simple repo frome remote.

    local_path: the path of repo at your localhost;
    remote:     the remote url of this repo;
    old_date:   the date of your local version.

    This function will sync the version after old_date of the repo
    node by node from remote.
    """
    # Clone repo from remote to /tmp
    print "git clone %s" % remote
    tempgit.clone(remote)
    curgit_label = remote.split('/')[-1][:-4]
    curgit_path = os.path.join(temproot, curgit_label)
    curgit = git.Git(curgit_path)
    print "clone to %s" % curgit_path

    # Generate patch for each node after old_date
    log_list = GetLogs(curgit, old_date)
    cur_date = log_list[0]['Date'] if log_list else old_date
    patchs = curgit.format_patch('-%d' % len(log_list))
    patch_list = patchs.splitlines()

    # Apply each patch
    prevdir = os.getcwd()
    os.chdir(os.path.join(root, local_path))
    localgit = git.Git('.')
    localgit.init()
    print "pushd %s" % os.chdir(os.path.join(root, local_path))
    for i in range(len(patch_list)):
        # pcmd = " patch -p1 < %s" % os.path.join(curgit_path, patch_list[i])
        # os.system(pcmd) # Call linux command `patch` here.
        localgit.apply(os.path.join(curgit_path, patch_list[i]))
        print "git apply %s" % os.path.join(curgit_path, patch_list[i])
        project.add(local_path)
        print " git add %s" % local_path
        message = '"Update[%s] %s"' % (local_path, log_list[-i-1]['message'])
        project.commit('-m', message)
        print " git commit -m %s" % message
        # project.push()
    os.chdir(prevdir)
    print "popd"

    # Modify date in version.json to cur_date, and return it
    return cur_date

def GetLogs(curgit, old_date):
    """
    Return a list of commit structures.
    """
    log = curgit.log('--after=%s' % old_date)
    commits = [c.splitlines() for c in 
               re.split(r'^commit.*', log, flags=re.MULTILINE)[1:]]
    log_list = [
                   {
                       'Author':   commit[0][8:],
                       'Date':     commit[1][8:],
                       'message':  commit[2][4:]
                   } for commit in [[cl for cl in c if cl != u'']
                                    for c in commits]
               ]
    return log_list

if __name__ == '__main__':
    info     = json.load(open("version.json"))
    temproot = tempfile.mkdtemp(prefix='seL4')
    tempgit  = git.Git(temproot)
    root     = os.path.join(os.getcwd(), '..')
    project  = git.Git(root)
    Sync()
