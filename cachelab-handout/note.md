### trace
```
# op addr, size
I 0400d7d4,8        # instruction load
 M 0421c7f0,4       # data modify(load then store)
 L 04f6b868,8       # data load
 S 7ff0005c8,8      # data store
```

each L or S operation cause at most 1 miss. M can result in 2 hits, or a miss + a hit + a possible eviction

### csim usage
```
./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile> 
```

 ./csim -v -s 2 -E 2 -b 3 -t traces/trans.trace
 ./csim-ref -v -s 2 -E 2 -b 3 -t traces/trans.trace