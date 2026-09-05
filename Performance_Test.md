# Performance Test

- Done with `wrk`
    1. 12 threads
    2. 400 concurrent connections
    3. 30 seconds


```bash
$ wrk -t12 -c400 -d30s http://localhost:8080/
Running 30s test @ http://localhost:8080/
  12 threads and 400 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    16.47ms    3.84ms  50.77ms   70.54%
    Req/Sec     2.00k   258.48     3.46k    68.23%
  717602 requests in 30.09s, 86.91MB read
Requests/sec:  23846.02
Transfer/sec:      2.89MB
```
