# Performance Test

- Done with `wrk`
    1. 12 threads
    2. 400 concurrent connections
    3. 30 seconds

- Initial testing

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

- Performance test after TLS redis + other features

```bash
$ wrk -t12 -c400 -d30s -s login_test.lua https://localhost:8080/api/users
Running 30s test @ https://localhost:8080/api/users
  12 threads and 400 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    76.06ms   11.77ms 214.34ms   85.39%
    Req/Sec   434.43     64.32   640.00     75.76%
  143176 requests in 30.08s, 23.08MB read
  Socket errors: connect 0, read 0, write 0, timeout 125
Requests/sec:   4759.64
Transfer/sec:    785.53KB
```

```lua
/*login_test.lua*/
wrk.method = "GET"
wrk.headers["Authorization"] = "Bearer <TOKEN>"
wrk.headers["Content-Type"] = "application/json"
```
