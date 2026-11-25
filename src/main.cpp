#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <functional>
#include <limits>
#include <list>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std;
using i64 = long long;
static constexpr i64 INF = (1LL << 60);

struct Input {
    static int ch() {
#ifdef _WIN32
        return _getchar_nolock();
#else
        return getchar_unlocked();
#endif
    }
    static string word() {
        string s;
        int c;
        do { c = ch(); } while (c != EOF && c <= ' ');
        while (c != EOF && c > ' ') { s.push_back((char)c); c = ch(); }
        return s;
    }
    static i64 number() {
        int c;
        do { c = ch(); } while (c != EOF && c <= ' ');
        bool neg = c == '-';
        if (neg) c = ch();
        i64 x = 0;
        while (c >= '0' && c <= '9') { x = x * 10 + c - '0'; c = ch(); }
        return neg ? -x : x;
    }
};

static i64 ceildiv(i64 a, i64 b) { return (a + b - 1) / b; }

struct LRU {
    i64 cap = 0, used = 0;
    list<int> order; // front: least recently used
    unordered_map<int, list<int>::iterator> pos;
    void clear() { order.clear(); pos.clear(); used = 0; }
    bool touch(int layer) {
        auto it = pos.find(layer);
        if (it == pos.end()) return false;
        order.splice(order.end(), order, it->second);
        return true;
    }
    bool contains(int layer) const { return pos.find(layer) != pos.end(); }
    void insert(int layer, const vector<i64>& size) {
        if (touch(layer)) return;
        order.push_back(layer);
        pos[layer] = prev(order.end());
        used += size[layer];
        while (used > cap) {
            int old = order.front();
            used -= size[old];
            pos.erase(old);
            order.pop_front();
        }
    }
};

struct Edge { int u, v; i64 w; bool up = true; };
struct Rack { int dc, node; LRU cache; };
struct Machine {
    int rack;
    i64 cpu, mem, freeCpu, freeMem;
    bool up = true;
    LRU cache;
    vector<int> running;
};
struct Tenant {
    int parent;
    i64 quota, used = 0;
    i64 settled = 0, rate = 0, intercept = 0;
};
struct Job {
    int id, wf, cpu, mem, est, img, inputRack;
    i64 inputSize, outputSize;
    vector<int> pred, succ;
    int remainingPred = 0;
    int state = 0; // 0 pending, 1 running, 2 done
    int machine = -1, machinePos = -1, start = -1, outputRack = -1;
    int tail = 0;
};
struct Workflow {
    int id, tenant, arrival, deadline, done = 0;
    i64 value, remainingWork = 0;
    bool dropped = false, completed = false;
    vector<int> jobs;
    int slackVersion = -1;
    vector<i64> slackCache;
};

struct AhoNode {
    array<int, 8> next{};
    int fail = 0;
    i64 score = 0;
    AhoNode() { next.fill(-1); }
};
struct HuffNode { int left = -1, right = -1, symbol = -1; i64 weight = 0; };

struct Scheduler {
    int T, D, N, R, M, K, L, I, P, W, E;
    i64 Breg, Brack, Bnet, Ureg, Unet;
    vector<i64> price, layerSize;
    vector<Rack> racks;
    vector<Machine> machines;
    vector<Tenant> tenants;
    vector<Edge> edges;
    vector<vector<pair<int, int>>> graph; // neighbor, edge id
    vector<bool> dcUp;
    vector<vector<int>> images;
    vector<HuffNode> huff;
    int huffRoot = -1;
    vector<AhoNode> aho;
    vector<deque<pair<int, i64>>> alarms;
    vector<i64> alarmSum;
    vector<pair<int, i64>> undo;
    vector<Job> jobs;
    vector<Workflow> workflows;
    unordered_map<int, int> jobById;
    vector<vector<i64>> distCache;
    vector<int> distVersion;
    int topoVersion = 1;
    int tick = 0;
    i64 minPrice = INF;

    void init() {
        T = (int)Input::number(); D = (int)Input::number(); N = (int)Input::number();
        R = (int)Input::number(); M = (int)Input::number(); K = (int)Input::number();
        L = (int)Input::number(); I = (int)Input::number(); P = (int)Input::number();
        W = (int)Input::number(); E = (int)Input::number();
        Breg = Input::number(); Brack = Input::number(); Bnet = Input::number();
        Ureg = Input::number(); Unet = Input::number();
        price.resize(D);
        for (auto& x : price) { x = Input::number(); minPrice = min(minPrice, x); }
        racks.resize(R);
        for (auto& r : racks) {
            r.dc = (int)Input::number(); r.node = (int)Input::number(); r.cache.cap = Input::number();
        }
        machines.resize(M);
        for (auto& m : machines) {
            m.rack = (int)Input::number(); m.cpu = Input::number(); m.mem = Input::number();
            m.cache.cap = Input::number(); m.freeCpu = m.cpu; m.freeMem = m.mem;
        }
        graph.resize(N); edges.resize(E);
        for (int e = 0; e < E; ++e) {
            auto& x = edges[e];
            x.u = (int)Input::number(); x.v = (int)Input::number(); x.w = Input::number();
            graph[x.u].push_back({x.v, e}); graph[x.v].push_back({x.u, e});
        }
        tenants.resize(K);
        for (auto& x : tenants) { x.parent = (int)Input::number(); x.quota = Input::number(); }
        layerSize.resize(L);
        for (auto& x : layerSize) x = Input::number();
        images.resize(I);
        for (auto& img : images) {
            int n = (int)Input::number(); img.resize(n);
            for (int& x : img) x = (int)Input::number();
        }
        buildHuffman(); buildAho();
        dcUp.assign(D, true);
        alarms.resize(M); alarmSum.assign(M, 0);
        distCache.resize(R); distVersion.assign(R, 0);
        jobs.reserve(60000); jobById.reserve(80000);
    }

    void buildHuffman() {
        using Item = pair<i64, int>;
        priority_queue<Item, vector<Item>, greater<Item>> pq;
        huff.reserve(17);
        for (int i = 0; i < 9; ++i) {
            i64 w = Input::number();
            HuffNode n; n.symbol = i; n.weight = w; huff.push_back(n);
            pq.push({w, i});
        }
        while (pq.size() > 1) {
            auto [wa, a] = pq.top(); pq.pop();
            auto [wb, b] = pq.top(); pq.pop();
            HuffNode n; n.left = a; n.right = b; n.weight = wa + wb;
            int id = (int)huff.size(); huff.push_back(n); pq.push({n.weight, id});
        }
        huffRoot = pq.top().second;
    }

    void buildAho() {
        aho.emplace_back();
        for (int i = 0; i < P; ++i) {
            string pat = Input::word(); i64 weight = Input::number();
            int node = 0;
            for (char c : pat) {
                int letter = c - 'a';
                if (aho[node].next[letter] == -1) {
                    int id = (int)aho.size(); aho[node].next[letter] = id; aho.emplace_back();
                }
                node = aho[node].next[letter];
            }
            aho[node].score += weight;
        }
        queue<int> q;
        for (int c = 0; c < 8; ++c) {
            int v = aho[0].next[c];
            if (v == -1) aho[0].next[c] = 0;
            else { aho[v].fail = 0; q.push(v); }
        }
        while (!q.empty()) {
            int u = q.front(); q.pop();
            aho[u].score += aho[aho[u].fail].score;
            for (int c = 0; c < 8; ++c) {
                int v = aho[u].next[c];
                if (v == -1) aho[u].next[c] = aho[aho[u].fail].next[c];
                else { aho[v].fail = aho[aho[u].fail].next[c]; q.push(v); }
            }
        }
    }

    int nodeDc(int node) const { return (int)((i64)node * D / N); }
    bool nodeUp(int node) const { return dcUp[nodeDc(node)]; }
    i64 distance(int a, int b) {
        int src = racks[a].node, dst = racks[b].node;
        if (!nodeUp(src) || !nodeUp(dst)) return INF;
        if (distVersion[a] != topoVersion) {
            auto& dist = distCache[a]; dist.assign(N, INF); dist[src] = 0;
            priority_queue<pair<i64, int>, vector<pair<i64, int>>, greater<pair<i64, int>>> pq;
            pq.push({0, src});
            while (!pq.empty()) {
                auto [d, u] = pq.top(); pq.pop();
                if (d != dist[u]) continue;
                for (auto [v, e] : graph[u]) {
                    if (!edges[e].up || !nodeUp(v)) continue;
                    i64 nd = d + edges[e].w;
                    if (nd < dist[v]) { dist[v] = nd; pq.push({nd, v}); }
                }
            }
            distVersion[a] = topoVersion;
        }
        return distCache[a][dst];
    }

    template <class F>
    void walkTenant(int t, F&& f) {
        for (int a = t; a != -1; a = tenants[a].parent) f(tenants[a]);
    }
    bool quotaFits(int t, int cpu) const {
        for (int a = t; a != -1; a = tenants[a].parent)
            if (tenants[a].used + cpu > tenants[a].quota) return false;
        return true;
    }
    i64 head(int t) const {
        i64 ans = INF;
        for (int a = t; a != -1; a = tenants[a].parent)
            ans = min(ans, tenants[a].quota - tenants[a].used);
        return ans;
    }
    void settle(int t, i64 amount) { walkTenant(t, [&](Tenant& a) { a.settled += amount; }); }
    void startAccounting(int t, int cpu, i64 rate, int now) {
        walkTenant(t, [&](Tenant& a) {
            a.used += cpu; a.rate += rate; a.intercept -= rate * now;
        });
    }
    void stopAccounting(int t, int cpu, i64 rate, int start, int now) {
        walkTenant(t, [&](Tenant& a) {
            a.used -= cpu; a.settled += rate * (now - start);
            a.rate -= rate; a.intercept += rate * start;
        });
    }

    void release(int j) {
        Job& job = jobs[j];
        int x = job.machine;
        Machine& m = machines[x];
        i64 rate = (i64)job.cpu * price[racks[m.rack].dc];
        stopAccounting(workflows[job.wf].tenant, job.cpu, rate, job.start, tick);
        m.freeCpu += job.cpu; m.freeMem += job.mem;
        int last = m.running.back();
        m.running[job.machinePos] = last; jobs[last].machinePos = job.machinePos;
        m.running.pop_back();
        job.machine = -1; job.machinePos = -1; job.start = -1; job.state = 0;
    }
    void done(int j) {
        Job& job = jobs[j];
        job.outputRack = machines[job.machine].rack;
        release(j); job.state = 2;
        Workflow& wf = workflows[job.wf];
        ++wf.done; wf.remainingWork -= (i64)job.cpu * job.est;
        if (wf.done == (int)wf.jobs.size()) wf.completed = true;
        for (int v : job.succ) --jobs[v].remainingPred;
    }
    void fail(int x) {
        Machine& m = machines[x]; m.up = false;
        while (!m.running.empty()) release(m.running.back());
        m.cache.clear();
    }

    void logLine(int x, int bits, const string& hex) {
        i64 score = 0;
        int node = huffRoot, ac = 0;
        for (int i = 0; i < bits; ++i) {
            char h = hex[i / 4];
            int nibble = h <= '9' ? h - '0' : (h | 32) - 'a' + 10;
            int bit = (nibble >> (3 - i % 4)) & 1;
            node = bit ? huff[node].right : huff[node].left;
            if (huff[node].symbol != -1) {
                int symbol = huff[node].symbol;
                if (symbol == 8) ac = 0;
                else { ac = aho[ac].next[symbol]; score += aho[ac].score; }
                node = huffRoot;
            }
        }
        auto& q = alarms[x];
        if (!q.empty() && q.back().first == tick) q.back().second += score;
        else q.push_back({tick, score});
        alarmSum[x] += score;
    }
    i64 alarm(int x) {
        auto& q = alarms[x];
        while (!q.empty() && q.front().first <= tick - W) {
            alarmSum[x] -= q.front().second; q.pop_front();
        }
        return alarmSum[x];
    }

    void addWorkflow() {
        Workflow wf;
        wf.id = (int)Input::number(); wf.tenant = (int)Input::number();
        wf.value = Input::number(); wf.deadline = (int)Input::number();
        int count = (int)Input::number(); wf.arrival = tick;
        int wi = (int)workflows.size(); workflows.push_back(move(wf));
        auto& stored = workflows.back(); stored.jobs.reserve(count);
        for (int i = 0; i < count; ++i) {
            Job job;
            job.id = (int)Input::number(); job.cpu = (int)Input::number();
            job.mem = (int)Input::number(); job.est = (int)Input::number();
            job.img = (int)Input::number(); job.inputRack = (int)Input::number();
            job.inputSize = Input::number(); job.outputSize = Input::number();
            int k = (int)Input::number(); job.wf = wi; job.remainingPred = k;
            for (int p = 0; p < k; ++p) {
                int id = (int)Input::number();
                job.pred.push_back(jobById.at(id));
            }
            int index = (int)jobs.size();
            jobById[job.id] = index;
            jobs.push_back(move(job)); stored.jobs.push_back(index);
            stored.remainingWork += (i64)jobs[index].cpu * jobs[index].est;
            for (int p : jobs[index].pred) jobs[p].succ.push_back(index);
        }
        for (int i = count - 1; i >= 0; --i) {
            Job& j = jobs[stored.jobs[i]];
            j.tail = j.est;
            for (int v : j.succ) j.tail = max(j.tail, j.est + jobs[v].tail);
        }
    }

    i64 slack(int j) {
        Workflow& wf = workflows[jobs[j].wf];
        if (wf.slackVersion == wf.done) return wf.slackCache[j - wf.jobs.front()];
        vector<i64> es(wf.jobs.size(), 0), tl(wf.jobs.size(), 0);
        auto offset = wf.jobs.front();
        i64 cp = 0;
        for (int u : wf.jobs) {
            if (jobs[u].state == 2) continue;
            for (int v : jobs[u].succ)
                if (jobs[v].state != 2)
                    es[v - offset] = max(es[v - offset], es[u - offset] + jobs[u].est);
        }
        for (int k = (int)wf.jobs.size() - 1; k >= 0; --k) {
            int u = wf.jobs[k];
            if (jobs[u].state == 2) continue;
            tl[u - offset] = jobs[u].est;
            for (int v : jobs[u].succ)
                if (jobs[v].state != 2)
                    tl[u - offset] = max(tl[u - offset], (i64)jobs[u].est + tl[v - offset]);
            cp = max(cp, es[u - offset] + jobs[u].est);
        }
        wf.slackCache.resize(wf.jobs.size());
        for (int u : wf.jobs)
            if (jobs[u].state != 2)
                wf.slackCache[u - offset] = cp - es[u - offset] - tl[u - offset];
        wf.slackVersion = wf.done;
        return wf.slackCache[j - offset];
    }

    bool inputTransfer(int j, int rack, i64& time, i64& fee) {
        Job& job = jobs[j];
        auto one = [&](int src, i64 size) {
            if (src == rack) return true;
            i64 d = distance(rack, src);
            if (d == INF) return false;
            time = max(time, d + ceildiv(size, Bnet));
            fee += size * d * Unet;
            return true;
        };
        if (job.inputRack != -1 && !one(job.inputRack, job.inputSize)) return false;
        for (int p : job.pred)
            if (!one(jobs[p].outputRack, jobs[p].outputSize)) return false;
        return true;
    }
    pair<i64, i64> pull(int j, int x) {
        Machine& m = machines[x]; LRU& mc = m.cache; LRU& rc = racks[m.rack].cache;
        i64 reg = 0, rack = 0;
        for (int layer : images[jobs[j].img]) {
            if (mc.touch(layer)) continue;
            if (rc.touch(layer)) rack += layerSize[layer];
            else { reg += layerSize[layer]; rc.insert(layer, layerSize); }
            mc.insert(layer, layerSize);
        }
        return {reg, rack};
    }
    i64 predictedRevenue(const Workflow& wf, int finish) const {
        int grace = max(5, (wf.deadline - wf.arrival) / 3);
        if (finish <= wf.deadline) return wf.value;
        if (finish >= wf.deadline + grace) return 0;
        return wf.value * (wf.deadline + grace - finish) / grace;
    }

    struct Choice { int machine = -1; i64 transferFee = 0; double score = 1e100; };
    Choice chooseMachine(int j) {
        const Job& job = jobs[j];
        const Workflow& wf = workflows[job.wf];
        Choice best;
        if (!quotaFits(wf.tenant, job.cpu)) return best;
        for (int x = 0; x < M; ++x) {
            const Machine& m = machines[x];
            if (!m.up || m.freeCpu < job.cpu || m.freeMem < job.mem) continue;
            i64 transferTime = 0, transferFee = 0;
            if (!inputTransfer(j, m.rack, transferTime, transferFee)) continue;
            i64 reg = 0, rack = 0;
            for (int layer : images[job.img]) {
                if (m.cache.contains(layer)) continue;
                if (racks[m.rack].cache.contains(layer)) rack += layerSize[layer];
                else reg += layerSize[layer];
            }
            i64 pullTime = ceildiv(reg, Breg) + ceildiv(rack, Brack);
            i64 setup = max(transferTime, pullTime);
            i64 finish = tick + setup + job.tail;
            i64 revenue = predictedRevenue(wf, (int)finish);
            if (!revenue || finish > T) continue;
            double delayPenalty = (double)wf.value / max(5, wf.deadline - wf.arrival) * setup;
            double cost = (double)job.cpu * price[racks[m.rack].dc] * (job.est + setup)
                        + transferFee + reg * Ureg + delayPenalty;
            if (cost < best.score) { best = {x, transferFee, cost}; }
        }
        return best;
    }
    void run(int j, int x, i64 transferFee) {
        Job& job = jobs[j]; Machine& m = machines[x];
        auto [reg, rack] = pull(j, x);
        int tenant = workflows[job.wf].tenant;
        settle(tenant, transferFee + reg * Ureg);
        startAccounting(tenant, job.cpu, (i64)job.cpu * price[racks[m.rack].dc], tick);
        m.freeCpu -= job.cpu; m.freeMem -= job.mem;
        job.state = 1; job.machine = x; job.start = tick;
        job.machinePos = (int)m.running.size(); m.running.push_back(j);
    }

    void event(const string& type) {
        if (type == "DONE") done(jobById.at((int)Input::number()));
        else if (type == "FAIL") fail((int)Input::number());
        else if (type == "UP") { int x = (int)Input::number(); machines[x].up = true; machines[x].cache.clear(); }
        else if (type == "DCDOWN" || type == "DCUP") {
            int d = (int)Input::number(); dcUp[d] = type == "DCUP"; ++topoVersion;
        } else if (type == "LDOWN" || type == "LUP") {
            int e = (int)Input::number(); edges[e].up = type == "LUP"; ++topoVersion;
        } else if (type == "QUOTA") {
            int t = (int)Input::number(); i64 q = Input::number();
            undo.push_back({t, tenants[t].quota}); tenants[t].quota = q;
        } else if (type == "UNDO") {
            int k = (int)Input::number();
            while (k--) { auto [t, q] = undo.back(); undo.pop_back(); tenants[t].quota = q; }
        } else if (type == "LOG") {
            int x = (int)Input::number(), bits = (int)Input::number();
            logLine(x, bits, Input::word());
        } else if (type == "WF") addWorkflow();
    }
    i64 answer(const string& q) {
        if (q == "BILL") {
            int o = (int)Input::number(); const auto& x = tenants[o];
            return x.settled + (i64)tick * x.rate + x.intercept;
        }
        if (q == "HEAD") return head((int)Input::number());
        if (q == "ROUTE") {
            int a = (int)Input::number(), b = (int)Input::number();
            i64 d = distance(a, b); return d == INF ? -1 : d;
        }
        if (q == "ALARM") return alarm((int)Input::number());
        if (q == "SLACK") return slack(jobById.at((int)Input::number()));
        if (q == "CACHE") return machines[(int)Input::number()].cache.used;
        if (q == "RCACHE") return racks[(int)Input::number()].cache.used;
        return 0;
    }
    void schedule(string& out) {
        struct Candidate { int j; double score; };
        vector<Candidate> ready;
        ready.reserve(jobs.size());
        for (int j = 0; j < (int)jobs.size(); ++j) {
            const Job& job = jobs[j];
            if (job.state != 0 || job.remainingPred != 0) continue;
            const Workflow& wf = workflows[job.wf];
            if (wf.dropped || wf.completed) continue;
            i64 revenue = predictedRevenue(wf, tick + job.tail);
            if (tick + job.tail > T || revenue <= 0 ||
                revenue * 5 < wf.remainingWork * minPrice * 4) continue;
            double urgency = 1.0 + 0.25 * max(0, 12 - (wf.deadline - tick - job.tail));
            double key = urgency * (double)revenue / max<i64>(1, wf.remainingWork * minPrice);
            ready.push_back({j, key});
        }
        size_t cap = min<size_t>(128, ready.size());
        partial_sort(ready.begin(), ready.begin() + cap, ready.end(),
                     [](const Candidate& a, const Candidate& b) {
                         if (a.score != b.score) return a.score > b.score;
                         return a.j < b.j;
                     });
        for (size_t i = 0; i < cap; ++i) {
            int j = ready[i].j;
            Choice choice = chooseMachine(j);
            if (choice.machine == -1) continue;
            out += "RUN " + to_string(jobs[j].id) + " " + to_string(choice.machine) + "\n";
            run(j, choice.machine, choice.transferFee);
        }
    }
    void loop() {
        init();
        for (tick = 1; tick <= T; ++tick) {
            string marker = Input::word();
            if (marker != "TICK") return;
            int incomingTick = (int)Input::number();
            int ne = (int)Input::number(), nq = (int)Input::number();
            tick = incomingTick;
            for (int i = 0; i < ne; ++i) event(Input::word());
            string out = "ANS";
            for (int i = 0; i < nq; ++i) {
                out += " " + to_string(answer(Input::word()));
            }
            out += "\n";
            schedule(out);
            out += "END\n";
            fwrite(out.data(), 1, out.size(), stdout);
            fflush(stdout);
        }
    }
};

int main() {
    Scheduler scheduler;
    scheduler.loop();
    return 0;
}
