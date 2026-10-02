# Usage: python3 summarize.py [LOGDIR] [TAG]. Median per case over the rounds in LOGDIR.
import sys, glob, statistics, collections
logdir = sys.argv[1] if len(sys.argv) > 1 else 'logs'
tag = sys.argv[2] if len(sys.argv) > 2 else 'round'
d = collections.defaultdict(lambda: collections.defaultdict(list))
for f in glob.glob(f'{logdir}/{tag}[0-9]*-*.txt'):
    build = f.rsplit('-', 1)[1][:-4]
    for line in open(f):
        p = line.strip().split('|')
        if len(p) != 5: continue
        d[(p[0], p[1], int(p[2]))][build].append(float(p[4]))
print(f"{'case':24} {'shape':6} {'n':>8} {'base us':>12} {'test us':>12} {'speedup':>8} rounds")
for k in sorted(d):
    m, b = d[k]['base'], d[k]['test']
    if not m or not b: continue
    mm, bm = statistics.median(m), statistics.median(b)
    print(f"{k[0]:24} {k[1]:6} {k[2]:>8} {mm:12.1f} {bm:12.1f} {mm/bm:7.2f}x {len(m)}/{len(b)}")
