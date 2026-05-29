#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <regex>
#include <algorithm>
#include <cctype>

using namespace std;
using ll = long long;

struct Coin { int d, c; };
struct Task { vector<Coin> wallet; ll amount; string strategy; };

struct DP {
    vector<unsigned char> can;
    vector<int> ps, pi, pc;
};

string readFile(const string& name) {
    ifstream in(name);
    stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void writeFile(const string& name, const vector<vector<pair<int,int >> >& ans) {
    ofstream out(name);
    out << "[\n";

    for (int i = 0; i < (int)ans.size(); i++) {
        out << "  {\n    \"dispense\": [";

        for (int j = 0; j < (int)ans[i].size(); j++) {
            if (j) out << ", ";
            out << "[" << ans[i][j].first << ", " << ans[i][j].second << "]";
        }

        out << "]\n  }";
        if (i + 1 < (int)ans.size()) out << ",";
        out << "\n";
    }

    out << "]\n";
}

string getArray(const string& obj, const string& key) {
    size_t p = obj.find("\"" + key + "\"");
    if (p == string::npos) return "[]";

    p = obj.find("[", p);

    int depth = 0;

    for (size_t i = p; i < obj.size(); i++) {
        if (obj[i] == '[') depth++;

        if (obj[i] == ']') {
            depth--;

            if (depth == 0) {
                return obj.substr(p, i - p + 1);
            }
        }
    }

    return "[]";
}

ll getNumber(const string& obj, const string& key) {
    size_t p = obj.find("\"" + key + "\"");
    p = obj.find(":", p) + 1;

    while (p < obj.size() && !isdigit((unsigned char)obj[p])) p++;

    ll x = 0;

    while (p < obj.size() && isdigit((unsigned char)obj[p])) {
        x = x * 10 + obj[p] - '0';
        p++;
    }

    return x;
}

string getString(const string& obj, const string& key) {
    size_t p = obj.find("\"" + key + "\"");
    p = obj.find(":", p);
    p = obj.find("\"", p);
    size_t q = obj.find("\"", p + 1);

    return obj.substr(p + 1, q - p - 1);
}

vector<Coin> parseWallet(const string& text) {
    vector<Coin> wallet;
    regex r(R"(\[\s*(\d+)\s*,\s*(\d+)\s*\])");

    for (auto it = sregex_iterator(text.begin(), text.end(), r);
         it != sregex_iterator(); ++it) {
        wallet.push_back({stoi((*it)[1]), stoi((*it)[2])});
    }

    return wallet;
}

vector<Task> parseTasks(const string& text) {
    vector<Task> tasks;
    regex objR(R"(\{[^{}]*\})");

    for (auto it = sregex_iterator(text.begin(), text.end(), objR);
         it != sregex_iterator(); ++it) {
        string obj = it->str();

        Task t;
        t.wallet = parseWallet(getArray(obj, "wallet"));
        t.amount = getNumber(obj, "amount");
        t.strategy = getString(obj, "strategy");

        tasks.push_back(t);
    }

    return tasks;
}

bool buildDP(const vector<Coin>& coins, const vector<int>& lim, int target, DP& dp) {
    dp.can.assign(target + 1, 0);
    dp.ps.assign(target + 1, -1);
    dp.pi.assign(target + 1, -1);
    dp.pc.assign(target + 1, 0);

    dp.can[0] = 1;

    for (int i = 0; i < (int)coins.size(); i++) {
        int left = lim[i];

        for (int part = 1; left > 0; part *= 2) {
            int take = min(part, left);
            int val = coins[i].d * take;

            for (int s = target; s >= val; s--) {
                if (!dp.can[s] && dp.can[s - val]) {
                    dp.can[s] = 1;
                    dp.ps[s] = s - val;
                    dp.pi[s] = i;
                    dp.pc[s] = take;
                }
            }

            left -= take;
        }
    }

    return dp.can[target];
}

bool restore(const vector<Coin>& coins, const DP& dp, int target, vector<int>& ans) {
    ans.assign(coins.size(), 0);

    if (target < 0 || target >= (int)dp.can.size() || !dp.can[target]) {
        return false;
    }

    for (int cur = target; cur > 0; cur = dp.ps[cur]) {
        if (dp.pi[cur] < 0) return false;
        ans[dp.pi[cur]] += dp.pc[cur];
    }

    return true;
}

bool bounded(
    const vector<Coin>& coins,
    ll amount,
    const vector<int>& low,
    const vector<int>& high,
    vector<int>& ans
) {
    int n = coins.size();

    ans.assign(n, 0);

    vector<int> lim(n);
    ll base = 0;

    for (int i = 0; i < n; i++) {
        int l = low[i];
        int h = min(high[i], coins[i].c);

        if (l > h) return false;

        ans[i] = l;
        lim[i] = h - l;
        base += 1LL * l * coins[i].d;
    }

    ll rest = amount - base;

    if (rest < 0 || rest > 2000000000LL) return false;
    if (rest == 0) return true;

    DP dp;

    if (!buildDP(coins, lim, (int)rest, dp)) return false;

    vector<int> add;

    if (!restore(coins, dp, (int)rest, add)) return false;

    for (int i = 0; i < n; i++) {
        ans[i] += add[i];
    }

    return true;
}

bool solveMaxMin(
    const vector<Coin>& coins,
    ll amount,
    bool maxBig,
    vector<int>& ans
) {
    int n = coins.size();
    int chosen = maxBig ? n - 1 : 0;

    vector<Coin> other;
    vector<int> pos;

    for (int i = 0; i < n; i++) {
        if (i != chosen) {
            other.push_back(coins[i]);
            pos.push_back(i);
        }
    }

    if (amount > 2000000000LL) return false;

    vector<int> lim(other.size());

    for (int i = 0; i < (int)other.size(); i++) {
        lim[i] = other[i].c;
    }

    DP dp;
    buildDP(other, lim, (int)amount, dp);

    int maxTake = min<ll>(coins[chosen].c, amount / coins[chosen].d);

    for (int take = maxTake; take >= 0; take--) {
        int rest = (int)(amount - 1LL * take * coins[chosen].d);

        if (rest >= 0 && dp.can[rest]) {
            vector<int> temp;
            restore(other, dp, rest, temp);

            ans.assign(n, 0);
            ans[chosen] = take;

            for (int i = 0; i < (int)temp.size(); i++) {
                ans[pos[i]] = temp[i];
            }

            return true;
        }
    }

    return false;
}

bool uniformDiff(
    const vector<Coin>& coins,
    ll amount,
    int diff,
    vector<int>& ans
) {
    int n = coins.size();
    int minCount = coins[0].c;

    for (auto c : coins) minCount = min(minCount, c.c);

    for (int left = 0; left <= minCount; left++) {
        vector<int> low(n), high(n);
        ll minSum = 0, maxSum = 0;

        for (int i = 0; i < n; i++) {
            low[i] = left;
            high[i] = min(coins[i].c, left + diff);

            minSum += 1LL * low[i] * coins[i].d;
            maxSum += 1LL * high[i] * coins[i].d;
        }

        if (minSum > amount) break;
        if (amount > maxSum) continue;

        if (bounded(coins, amount, low, high, ans)) {
            return true;
        }
    }

    return false;
}

bool solveUniform(const vector<Coin>& coins, ll amount, vector<int>& ans) {
    int maxCount = 0;

    for (auto c : coins) maxCount = max(maxCount, c.c);

    vector<int> temp;

    if (!uniformDiff(coins, amount, maxCount, temp)) {
        return false;
    }

    int l = 0, r = maxCount;

    while (l < r) {
        int m = (l + r) / 2;

        if (uniformDiff(coins, amount, m, temp)) {
            r = m;
        } else {
            l = m + 1;
        }
    }

    return uniformDiff(coins, amount, l, ans);
}

vector<pair<int,int>> solve(Task t) {
    sort(t.wallet.begin(), t.wallet.end(), [](Coin a, Coin b) {
        return a.d < b.d;
    });

    vector<int> cnt;
    bool ok = false;

    if (t.strategy == "MAX") {
        ok = solveMaxMin(t.wallet, t.amount, true, cnt);
    } else if (t.strategy == "MIN") {
        ok = solveMaxMin(t.wallet, t.amount, false, cnt);
    } else if (t.strategy == "UNIFORM") {
        ok = solveUniform(t.wallet, t.amount, cnt);
    }

    vector<pair<int,int>> res;

    if (!ok) return res;

    for (int i = 0; i < (int)t.wallet.size(); i++) {
        if (cnt[i] > 0) {
            res.push_back({t.wallet[i].d, cnt[i]});
        }
    }

    return res;
}

int main() {
    vector<Task> tasks = parseTasks(readFile("input.json"));
    vector<vector<pair<int,int>>> answers;

    for (auto t : tasks) {
        answers.push_back(solve(t));
    }

    writeFile("output.json", answers);

    return 0;
}