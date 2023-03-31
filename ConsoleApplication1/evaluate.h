#pragma once
#include <string>
#include<vector>
#include<stack>
#include<unordered_map>
using namespace std;

string eval(string expres)
{

    vector<string> polishExp;
    stack<char> stck;
    unordered_map<char, int> order{ {'*',3},{'+',1},{'-',1},{'(',0} };
    bool dig = true;
    bool oper = false;
    for (int i = 0; i < expres.size();)
    {
        if (expres[i] == ' ')
        {
            while (i < expres.size() && expres[i] == ' ')
                i++;
            continue;
        }
        if (dig)
        {
            while (i < expres.size() && expres[i] == ' ')
                i++;
            int j = i;
            if (expres[i] == '(')
            {
                stck.push('(');
                i++;
            }
            else if (isdigit(expres[i]) || expres[i] == '-')
            {
                try
                {
                    string a = expres.substr(i);
                    int b = stoll(a);
                    a = to_string(b);

                    polishExp.push_back(a);
                    i += a.size();
                    dig = false;
                    oper = true;
                }
                catch (...)
                {
                    return "ERROR:too big number pos:" + to_string(j);
                }
            }
            else
            {
                return "ERROR:unexpected character in pos:" + to_string(i);
            }
        }
        else if (oper)
        {
            if (expres[i] == '-' || expres[i] == '+' || expres[i] == '*')
            {
                while (!stck.empty() && order[stck.top()] >= order[expres[i]])
                {
                    std::string a = "";
                    a += stck.top();
                    polishExp.push_back(a);
                    stck.pop();
                }
                stck.push(expres[i]);
                oper = false;
                dig = true;
                i++;
            }
            else if (expres[i] == ')')
            {
                while (!stck.empty() && stck.top() != '(')
                {
                    string a = "";
                    a += stck.top();
                    polishExp.push_back(a);
                    stck.pop();
                }
                if (stck.empty())
                {
                    return  "ERROR:bracket not closed pos:" + to_string(i);
                }
                stck.pop();
                i++;
            }
            else
            {
                return "ERROR:unexpected character in pos:" + to_string(i);

            }
        }
    }
    if (dig)
    {
        return "ERROR:extra operand on the end";
    }
    while (!stck.empty())
    {
        if (stck.top() == '(')
        {
            return "ERROR:missed closed barcket";
        }
        string a = "";
        a += stck.top();
        polishExp.push_back(a);
        stck.pop();
    }
    stack<long long> polish;

    for (auto a : polishExp)
    {
        if (isdigit(a[0]) || (a[0] == '-' && a.size() > 1))
        {
            polish.push(stoll(a));
        }
        else if (a[0] == '+')
        {
            int res = polish.top();
            polish.pop();
            res += polish.top();
            polish.pop();
            polish.push(res);
        }
        else if (a[0] == '*')
        {
            int res = polish.top();
            polish.pop();
            res *= polish.top();
            polish.pop();
            polish.push(res);
        }
        else if (a[0] == '-')
        {
            int res = polish.top();
            polish.pop();
            res = polish.top() - res;
            polish.pop();
            polish.push(res);
        }
    }
    return to_string(polish.top());
}
