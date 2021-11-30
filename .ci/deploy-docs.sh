#!/bin/bash

set -x
set -o errexit -o nounset

if test "x$GITHUB_REF_TYPE" != xtag; then exit; fi


git config user.name "CI"
git config user.email "ci@raqm.org"
git fetch origin
git checkout -b gh-pages -t origin/gh-pages

cp build/docs/html/* .
rm -rf build
ls *
git add -A .

if [[ $(git status -s) ]]; then
  git commit -m "Rebuild docs for $GITHUB_REF"
  git push -q origin HEAD:gh-pages
fi
