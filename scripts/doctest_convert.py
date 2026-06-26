#!/usr/bin/env python3
"""Convert hand-rolled assert()/main() tests to doctest TEST_CASEs (in place).
Usage: doctest_convert.py <file.cpp> [file.cpp ...]
Prints JSON: {"converted":[...], "skipped_argv":[...], "nonzero_returns":[...]}.
Mechanical + proven on the math+scene cluster; argv-main files are skipped
(can't be argless TEST_CASEs) and reported for the caller to keep separate."""
import re, sys, json, pathlib

def wrap_checks(t):
    out=[]; i=0; n=len(t)
    while i<n:
        if t.startswith('CHECK(',i) and (i==0 or not(t[i-1].isalnum() or t[i-1]=='_')):
            j=i+6; depth=1
            while j<n and depth>0:
                if t[j]=='(':depth+=1
                elif t[j]==')':depth-=1
                j+=1
            arg=t[i+6:j-1].strip()
            out.append('CHECK('+arg+')' if (arg.startswith('(') and arg.endswith(')')) else 'CHECK(('+arg+'))')
            i=j
        else:
            out.append(t[i]); i+=1
    return ''.join(out)

def convert(path):
    p=pathlib.Path(path); s=p.read_text(); stem=p.stem
    if re.search(r'int\s+main\s*\(\s*int', s):
        return "skipped_argv"
    nonzero = bool(re.search(r'\breturn\s+[1-9]', s))
    if '#include <cassert>' in s:
        s=s.replace('#include <cassert>','#include "doctest/doctest.h"',1)
    else:
        s='#include "doctest/doctest.h"\n'+s
    s=re.sub(r'int\s+main\s*\(\s*(?:void)?\s*\)\s*\{', f'TEST_CASE("{stem}") {{', s, count=1)
    s=re.sub(r'\bassert\(','CHECK(',s)
    s=re.sub(r'\breturn\s+0\s*;','return;',s)
    s=wrap_checks(s)
    p.write_text(s)
    return "nonzero_returns" if nonzero else "converted"

res={"converted":[],"skipped_argv":[],"nonzero_returns":[]}
for f in sys.argv[1:]:
    res[convert(f)].append(f)
print(json.dumps(res,indent=2))
