#!/usr/bin/env python

from stridx import StringIndex
e=StringIndex()
e.set_value(3)
e.add("./rust/alloc/vec/spec_extend.rs",0)
e.add("./virt/kvm/dirty_ring.c",1)
e.add("./Documentation/staging/static-keys.rst",2)
e.add("./Documentation/staging/lzo.rst",3)



results = e.find("rstalloc")
for x in results:
	print(x)

# print(e.get_value())

