"""Deterministic A stress oracle; independent of the generator distribution."""

def fixture():
    endpoints=[
        '/v2/orders','/v2/orders-export','/v2/orders.csv','/v2/orders/{id}',
        '/v2/orders/{id}/items','/v2/ordersByDate','/v2/orders_legacy',
        '/v2/orderstatus','/v2/rank100','/v2/rank20','/','/health',
        '/internal/v1/tenants/{tenant_id}/projects/{project_id}/pipelines/{pipeline_id}/runs/{run_id}/artifacts/{artifact_id}:download',
    ]
    assert len(endpoints[-1])==125
    counts=[1,2,3,63,64,65,99,100,101,199,200,201,257]
    hours=['2026-10-30T18','2026-10-30T19','2026-10-31T23','2026-11-01T00','2026-11-02T17']
    methods=['GET','POST','PUT','PATCH','DELETE','HEAD','OPTIONS']
    anchors=[0,4294967295,4294967295,1,0,999999,2147483648]
    groups={};rows=[];statuses=set()
    for hi,hour in enumerate(hours):
        for ei,(endpoint,n) in enumerate(zip(endpoints,counts)):
            values=[];local=[]
            for j in range(n):
                latency=anchors[(j*7+ei)%len(anchors)] if j%3==0 else (j*2654435761+ei*104729+hi*17)&0xffffffff
                status=100+(hi*257+ei*79+j*31)%500
                if j<4:status=[499,500,599,200][j]
                ts=f'{hour}:{j*17%60:02d}:{j*13%60:02d}.{j*7919%1000000:06d}Z'
                if hi==4 and ei==12 and j==n-1:ts=hour+':59:59.999999Z'
                method=methods[(j+ei+hi)%len(methods)]
                if j and j%19==0:
                    line,latency,status=local[-1]
                else:line=f'{ts} {method} {endpoint} {status} {latency}\n'.encode()
                local.append((line,latency,status));rows.append(line);values.append((latency,status));statuses.add(status)
            groups[(hour,endpoint)]=values
    assert len(rows)==6775 and len(statuses)==500
    order=sorted(range(len(rows)),key=lambda i:(i*1103515245+12345)&0xffffffff)
    records=b''.join(rows[i] for i in order)
    expected=[]
    for (hour,endpoint),values in sorted(groups.items()):
        latencies=sorted(value for value,status in values);n=len(values)
        errors=sum(500<=status<=599 for value,status in values)
        ranks=[latencies[(percentile*n+99)//100-1] for percentile in [50,95,99]]
        expected.append(f'{hour}:00:00Z {endpoint} {n} {ranks[0]} {ranks[1]} {ranks[2]} {errors} {errors*1000000//n}\n')
    return records,''.join(expected).encode()

