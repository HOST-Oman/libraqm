#!/bin/bash

set -o errexit -o nounset

if test "x$TRAVIS_SECURE_ENV_VARS" != "xtrue"; then exit; fi

TAG="$(git describe --exact-match --match "v[0-9]*" HEAD 2>/dev/null || true)"

if test "x$TAG" = x; then exit; fi

DOCSDIR=build-docs

rm -rf $DOCSDIR || exit
mkdir $DOCSDIR
cd $DOCSDIR

cp ../docs/html/* .

git init
git config user.name "Travis CI"
git config user.email "travis@raqm.org"
set +x
echo "git remote add upstream \"https://\$GH_TOKEN@github.com/$TRAVIS_REPO_SLUG.git\""
git remote add upstream "https://$GH_TOKEN@github.com/$TRAVIS_REPO_SLUG.git"
set -x
git fetch upstream
git reset upstream/gh-pages

touch .
git add -A .
git commit -m "Rebuild docs for $TAG"
git push -q upstream HEAD:gh-pages
