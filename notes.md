Keep hot loops and their helpers in the same DLL (e.g., resource uploads, command buffer recording, descriptor updates, draw submission helpers all in engine_core or engine_render).
Make DLL APIs coarse-grained: “record frame,” “upload model batch,” “build pipeline,” not per-element or per-descriptor calls.
If you need shared math or small utilities, either:
build them as a static lib linked into each DLL (allows inlining), or
keep them in the same DLL as the hot callers.
For unavoidable cross-DLL calls in hot paths, reduce frequency (batch work), avoid tiny leaf functions, and prefer data buffers over many fine-grained calls.