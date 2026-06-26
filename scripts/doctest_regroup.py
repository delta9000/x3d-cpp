#!/usr/bin/env python3
"""Pre-register a grouped doctest binary for a subsystem's tests in CMakeLists.txt.
Removes the per-file add_executable/add_test/set_tests_properties blocks for the
subsystem's tests (except argv-main tests, which stay separate), and appends one
grouped target. Deterministic single-writer edit. Usage:
  doctest_regroup.py <subsys> <grouped_target> [--apply]
Without --apply, prints the plan (dry-run)."""
import re, sys, pathlib, json

subsys, gtarget = sys.argv[1], sys.argv[2]
apply = "--apply" in sys.argv
cm = pathlib.Path("CMakeLists.txt")
lines = cm.read_text().split('\n')

# Pass 1: find add_executable blocks whose source is runtime/<subsys>/tests/*.cpp
blocks=[]  # {start, end(add_test line idx), target, testname, src, defs[]}
i=0
while i < len(lines):
    m=re.search(r'add_executable\((\S+)', lines[i])
    if m:
        tgt=m.group(1); start=i; src=None; defs=[]; testname=None; j=i
        while j < len(lines) and 'add_test(' not in lines[j]:
            sm=re.search(rf'runtime/{subsys}/tests/([^"]+\.cpp)', lines[j])
            if sm: src=sm.group(0)
            dd=re.findall(r'"([^"]*X3D_[A-Z0-9_]+=[^"]*)"', lines[j])
            defs+=dd
            j+=1
        # j now at the add_test line; balance parens to find its end (may be multi-line)
        atxt=""; k=j; depth=0; started=False
        while k < len(lines):
            atxt+=lines[k]+"\n"
            depth+=lines[k].count('(')-lines[k].count(')')
            if '(' in lines[k]: started=True
            if started and depth<=0: break
            k+=1
        nm=re.search(r'add_test\(NAME\s+(\S+)', atxt); testname=nm.group(1) if nm else None
        # argv-dependent if there is any token after "COMMAND <target>" before close
        cm2=re.search(r'COMMAND\s+\S+\s+(\S)', atxt); has_args=bool(cm2)
        if src:
            blocks.append({"start":start,"end":k,"target":tgt,"testname":testname,"src":src,"defs":defs,"argv_cmd":has_args})
        i=k+1
    else:
        i+=1

# classify argv-main (keep separate)
group=[]; keep_argv=[]
for b in blocks:
    txt=pathlib.Path(b["src"]).read_text() if pathlib.Path(b["src"]).exists() else ""
    if re.search(r'int\s+main\s*\(\s*int', txt) or b.get("argv_cmd"): keep_argv.append(b)
    else: group.append(b)

plan={"subsystem":subsys,"grouped_target":gtarget,
      "group_count":len(group),"argv_kept_separate":[b["target"] for b in keep_argv],
      "compile_defs":sorted({d for b in group for d in b["defs"]}),
      "sources":[b["src"] for b in group]}
print(json.dumps(plan,indent=2))
if not apply:
    sys.exit(0)

# remove grouped blocks (add_executable..add_test inclusive) + their set_tests_properties
remove_ranges=set()
remove_testnames={b["testname"] for b in group if b["testname"]}
for b in group:
    for k in range(b["start"], b["end"]+1): remove_ranges.add(k)
out=[]
for idx,l in enumerate(lines):
    if idx in remove_ranges: continue
    if re.match(r'\s*set_tests_properties\((\S+)', l) and re.match(r'\s*set_tests_properties\((\S+)', l).group(1) in remove_testnames: continue
    out.append(l)
srcs="\n".join(f'        "${{CMAKE_CURRENT_SOURCE_DIR}}/{b["src"]}"' for b in group)
defs_lines=""
if plan["compile_defs"]:
    dl="\n".join(f'        "{d}"' for d in plan["compile_defs"])
    defs_lines=f'    target_compile_definitions({gtarget} PRIVATE\n{dl})\n'
block=f'''
# --- doctest grouped binary: {subsys} tests ({len(group)} cases, one link) ---
if(X3D_CPP_BUILD_TESTS AND TARGET x3d_cpp_nodes)
    add_executable({gtarget}
        "${{CMAKE_CURRENT_SOURCE_DIR}}/runtime/test_support/doctest_main.cpp"
{srcs})
    target_link_libraries({gtarget} PRIVATE x3d_cpp::sdk)
    target_include_directories({gtarget} PRIVATE
        "${{CMAKE_CURRENT_SOURCE_DIR}}/runtime/test_support")
{defs_lines}    add_test(NAME {gtarget} COMMAND {gtarget})
    set_tests_properties({gtarget} PROPERTIES TIMEOUT 180)
endif()
'''
cm.write_text("\n".join(out).rstrip()+"\n"+block)
print("APPLIED")
