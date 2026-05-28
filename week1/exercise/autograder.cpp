#include <bits/stdc++.h>
using namespace std;
#define int long long
/*
    Compile: g++ -O2 autograder_pick12.cpp -o autograder
    Run: ./autograder

    Students should fill only the solve function below.
*/

string solve(int n, vector<long long> a) {
    // TODO: Fill this function.
    // Return one of: "Player 1" or "Player 2" or "Draw"
    vector<int> ans(n);
    vector<int> totm1(n);
    vector<int> totm2(n);
    totm1[n-1]=a[n-1];
    if(a[n-1]>0)
    {
        ans[n-1] = 1;
    }
    else if (a[n-1]<0)
    {
        ans[n-1]=2;
    }
    else
    {
        ans[n-1]=0;
    }
    int s = a[n-1];
    if(n!=1)
    {
        s+=a[n-2];
        int x = a[n-1]+a[n-2];
        if(x>0 || a[n-2]>a[n-1])
        {
            totm1[n-2]=max(x, a[n-2]);
            totm2[n-2]=x-totm1[n-2];
            ans[n-2]=1;
        }
        else if (a[n-2]<a[n-1] && x<0)
        {
            totm1[n-2]=max(x, a[n-2]);
            totm2[n-2]=x-totm1[n-2];
            ans[n-2]=2;
        }
        else
        {
            totm1[n-2]=max(x, a[n-2]);
            totm2[n-2]=x-totm1[n-2];
            ans[n-2]=0;
        }
    }

    for(int i=n-3; i>=0; i--)
    {
        s+=a[i];
        int o = 0; int t = 0;
        o = totm2[i+1]+a[i] - totm1[i+1];
        t = totm2[i+2]+a[i]+a[i+1] - totm1[i+2];
        if(o>t)
        {
            totm1[i]=totm2[i+1]+a[i];
            totm2[i]=totm1[i+1];
        }
        else
        {
            totm1[i]=totm2[i+2]+a[i]+a[i+1];
            totm2[i]=totm1[i+2];
        }
    }

    if(totm1[0]>totm2[0])
    {
        return "Player 1";
    }
    else if (totm1[0]<totm2[0])
    {
        return "Player 2";
    }
    else
    {
        return "Draw";
    }
    

    return "";
}

static string trim(const string &s) {
    int l = 0, r = (int)s.size() - 1;
    while (l <= r && isspace((unsigned char)s[l])) l++;
    while (r >= l && isspace((unsigned char)s[r])) r--;
    if (l > r) return "";
    return s.substr(l, r - l + 1);
}

static bool file_exists(const string &path) {
    ifstream f(path);
    return f.good();
}

signed main() {
    const string folder = "testcases";

    int passed = 0;
    int total = 0;

    for (int tc = 0; ; tc++) {
        string in_path = folder + "/input" + to_string(tc) + ".txt";
        string out_path = folder + "/output" + to_string(tc) + ".txt";

        if (!file_exists(in_path) && !file_exists(out_path)) {
            break;
        }

        total++;

        if (!file_exists(in_path)) {
            cout << "Failed on testcase " << tc << ": missing " << in_path << '\n';
            continue;
        }
        if (!file_exists(out_path)) {
            cout << "Failed on testcase " << tc << ": missing " << out_path << '\n';
            continue;
        }

        ifstream fin(in_path);
        int n;
        fin >> n;
        vector<long long> a(n);
        for (int i = 0; i < n; i++) fin >> a[i];

        if (!fin) {
            cout << "Failed on testcase " << tc << ": invalid input format\n";
            continue;
        }

        string got = trim(solve(n, a));

        ifstream fout(out_path);
        string expected, line;
        while (getline(fout, line)) {
            if (!expected.empty()) expected += "\n";
            expected += line;
        }
        expected = trim(expected);

        if (got == expected) {
            cout << "OK testcase " << tc << '\n';
            passed++;
        } else {
            cout << "Failed on testcase " << tc << '\n';
            cout << "Expected: " << expected << '\n';
            cout << "Got     : " << got << '\n';
        }
    }

    if (total == 0) {
        cout << "No testcases found. Make sure there is a testcases folder with input0.txt and output0.txt.\n";
        return 1;
    }

    cout << "\nPassed " << passed << " / " << total << " testcases.\n";

    if (passed == total) {
        cout << "OK\n";
        return 0;
    } else {
        cout << "Failed\n";
        return 1;
    }
}
