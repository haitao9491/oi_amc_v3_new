#!/bin/sh
#
# To execute git commands on the repository and all it's subordinates.
#
# (C) Copyright 2010
# Hu Chunlin <chunlin.hu@gmail.com>
#

ECHO="/bin/echo"

export TERM=xterm

HIGHLIGHT="`tput setaf 4``tput bold`"
APPNAME="`tput setab 4``tput setaf 7`"
NORMAL="`tput sgr0`"

indentstr=""

if [ -z $1 ]; then
	command=status
else
	command=$1
	shift
fi
args=$*

git_status() {
	clear
	${ECHO} "${HIGHLIGHT}Repository: ${APPNAME}$1${NORMAL}"

	git status
	${ECHO}

	${ECHO} -n "${HIGHLIGHT}Press any key to continue to next repository.${NORMAL}"
	read key
}

git_rebase() {
	${ECHO} "${HIGHLIGHT}Repository: ${APPNAME}$1${NORMAL}"

	git checkout master
	git pull
	git checkout mywork
	git rebase origin
	${ECHO}
}

git_show_branch() {
	clear
	${ECHO} "${HIGHLIGHT}Repository: ${APPNAME}$1${NORMAL}"

	git show-branch
	${ECHO}

	${ECHO} -n "${HIGHLIGHT}Press any key to continue to next repository.${NORMAL}"
	read key
}

git_format_patch() {
	patched=0

	git format-patch remotes/origin/master >/dev/null
	for patchfile in `ls *.patch 2>/dev/null`
	do
		${ECHO} -n "${indentstr}"
		${ECHO} "$1" | awk '{printf("%-16s", $1);}'
		${ECHO} ${patchfile}
		mv ${patchfile} /tmp/$1_${patchfile}
		patched=1
	done

	if [ $patched -eq 0 ]; then
		${ECHO} -n "${indentstr}"
		${ECHO} "$1" | awk '{printf("%-16s", $1);}'
		${ECHO}
	fi
}

git_apply_patch() {
	patched=0

	git checkout master >/dev/null 2>/dev/null
	for patchfile in `ls /tmp/$1_[0-9][0-9][0-9][0-9]-*.patch 2>/dev/null`
	do
		${ECHO} -n "${indentstr}"
		${ECHO} "$1" | awk '{printf("%-16s", $1);}'
		${ECHO} "`basename ${patchfile}`"
		git am -3su $patchfile
		patched=1
	done

	if [ $patched -eq 0 ]; then
		${ECHO} -n "${indentstr}"
		${ECHO} "$1" | awk '{printf("%-16s", $1);}'
		${ECHO}
	fi
}

git_branch() {
	${ECHO} -n "${indentstr}"
	${ECHO} "$1" | awk '{printf("%-16s", $1);}'
	git branch | grep "^\*"
}

git_checkout() {
	${ECHO} -n "${indentstr}"
	${ECHO} "$1" | awk '{printf("%-16s", $1);}'
	git checkout ${args}
}

git_reset() {
	${ECHO} -n "${indentstr}"
	${ECHO} "$1" | awk '{printf("%-16s", $1);}'
	git reset ${args}
}

git_pull() {
	clear
	${ECHO} "${HIGHLIGHT}Repository: ${APPNAME}$1${NORMAL}"

	git pull
	${ECHO}

	${ECHO} -n "${HIGHLIGHT}Press any key to continue to next repository.${NORMAL}"
	read key
}

git_push() {
	${ECHO} -n "${indentstr}"
	${ECHO} "$1" | awk '{printf("%-16s", $1);}'
	git push origin master
}

git_cmd() {
	${ECHO} -n "${indentstr}"
	${ECHO} "$1" | awk '{printf("%-16s\n", $1);}'
	git ${args}
}

do_command() {
	currdir=`pwd`
	repo=`basename ${currdir}`

	case "${command}" in
		status)
			git_status ${repo}
			;;

		rebase)
			git_rebase ${repo}
			;;

		show-branch)
			git_show_branch ${repo}
			;;

		format-patch)
			git_format_patch ${repo}
			;;

		apply-patch)
			git_apply_patch ${repo}
			;;

		branch)
			git_branch ${repo}
			;;

		checkout)
			git_checkout ${repo}
			;;

		reset)
			git_reset ${repo}
			;;

		pull)
			git_pull ${repo}
			;;

		push)
			git_push ${repo}
			;;

		cmd)
			git_cmd ${repo}
			;;

		*)
			${ECHO} "Usage: $0 {status|rebase|show-branch|format-patch|apply-patch|branch|checkout|reset|push|cmd} [args]"
			exit 1
			;;
	esac
}

recursion() {
	if grep -Eqe "^SUBDIRS = " Makefile; then
		l1repo="`grep -Ee "^SUBDIRS = " Makefile | awk -F= '{print $2;}'`"
		indentstr="${indentstr}  "
		for dir in ${l1repo}
		do
			if [ -d ${dir} ]; then
				cd ${dir}
				if [ -d .git ]; then
					do_command
					recursion
				fi
				cd ..
			fi
		done
		indentstr="`${ECHO} \"${indentstr}\" | cut -c3-`"
	fi
}

do_command
recursion

