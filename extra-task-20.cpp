#include <algorithm>
#include <iostream>
#include <cassert>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <map>

#define debug(x) std::cout << #x << " = " << x << std::endl

struct FSA {
    std::map<std::string, int> Σ;
    std::map<std::string, int> Q;
    std::vector<std::vector<int>> δ;
    std::vector<bool> F; // F[i] == true  <=>  i - допускающее состояние
    int s; // Начальное состояние
};

void print_fsa_to_stdout(const FSA &a) {
    for (auto [k, v] : a.Σ) {
        std::cout << "(" << k << "," << v << ") ";
    }
    std::cout << "\n";
    for (auto [k, v] : a.Q) {
        std::cout << "(" << k << "," << v << ") ";
    }
    std::cout << "\n";

    for (int i = 0; i < a.δ.size(); i++) {
        for (int j = 0; j < a.Σ.size(); j++) {
            std::cout << a.δ[i][j] << " ";
        }

        if (a.F[i]) {
            std::cout << "! ";
        }
        if (a.s == i) {
            std::cout << "*";
        }
        std::cout << "\n";
    }
}

template <typename T>
std::ostream &operator <<(std::ostream &os, const std::set<T> &s) {
    if (s.size() == 0) {
        os << "{}";
        return os;
    }

    int i = 0;
    auto it = s.begin();

    os << "{";
    for (; i < s.size() - 1; it++, i++) {
        os << *it << " ";
    }
    os << *it << "}";

    return os;
}

FSA minimize(const FSA &a) {
    std::set<int> final_states;
    std::set<int> non_final_states;
    for (int i = 0; i < a.δ.size(); i++) {
        if (a.F[i]) {
            final_states.insert(i);
        } else {
            non_final_states.insert(i);
        }
    }

    std::set<std::set<int>> P;
    std::set<std::set<int>> W;
    if (final_states.size() > 0) {
        P.insert(final_states);
        W.insert(final_states);
    }
    if (non_final_states.size() > 0) {
        P.insert(non_final_states);
        W.insert(non_final_states);
    }
    while (!W.empty()) {
        auto first = W.begin();
        std::set<int> A = *first;
        W.erase(first);

        for (int letter = 0; letter < a.Σ.size(); letter++) {
            std::set<int> X;
            for (int row = 0; row < a.δ.size(); row++) {
                if (A.find(a.δ[row][letter]) != A.end()) {
                    X.insert(row);
                }
            }

            auto it = P.begin();
            while (it != P.end()) {
                const std::set<int> &Y = *it;

                std::set<int> intersection;
                auto intersection_inserter = std::inserter(intersection, intersection.begin());
                std::set_intersection(X.begin(), X.end(), Y.begin(), Y.end(), intersection_inserter);

                std::set<int> difference;
                auto difference_inserter = std::inserter(difference, difference.begin());
                std::set_difference(Y.begin(), Y.end(), X.begin(), X.end(), difference_inserter);

                if (intersection.empty() || difference.empty()) {
                    it++;
                    continue;
                }

                P.erase(it);
                P.insert(intersection);
                P.insert(difference);

                if (W.find(Y) != W.end()) {
                    W.erase(Y);
                    W.insert(intersection);
                    W.insert(difference);
                } else {
                    if (intersection.size() <= difference.size()) {
                        W.insert(intersection);
                    } else {
                        W.insert(difference);
                    }
                }

                it = P.begin();
            }
        }
    }

    FSA a_min;
    a_min.Σ = a.Σ;
    a_min.δ = std::vector<std::vector<int>>(P.size(), std::vector<int>(a.Σ.size(), -1));
    a_min.F = std::vector<bool>(P.size(), false);
    a_min.s = -1;

    std::map<int, int> state_map;
    int state_id = 0;
    for (const auto &p: P) {
        for (auto e: p) {
            state_map[e] = state_id;
        }
        if (p.size() != 0) {
            state_id += 1;
        }
    }

    for (const auto &p: P) {
        int representative = *p.begin();
        int new_state = state_map[representative];
        for (int letter = 0; letter < a.Σ.size(); letter++) {
            int next_state = a.δ[representative][letter];
            a_min.δ[new_state][letter] = state_map[next_state];
        }
        a_min.F[new_state] = a.F[representative];
        a_min.Q[std::to_string(new_state)] = new_state;
    }
    a_min.s = state_map[a.s];

    assert(a_min.s != -1);

    return a_min;
}

FSA patch_up_fsa(const FSA &a) {
    FSA copy = a;
    for (int i = 0; i < copy.δ.size(); i++) {
        for (int j = 0; j < copy.δ[i].size(); j++) {
            if (copy.δ[i][j] == -1) {
                copy.δ[i][j] = 0;
            }
        }
    }

    copy = minimize(copy);

    return copy;
}

std::string find_by_value(const std::map<std::string, int> &m, int value) {
    for (auto [k, v] : m) {
        if (v == value) {
            return k;
        }
    }
    assert(false);
}

void print_fsa_to_dot_file(const FSA &a, const std::string &filename) {
    std::ofstream ofs(filename);

    ofs << "digraph a {\n";
    ofs << "    rankdir = LR;\n";
    for (auto [k, v] : a.Q) {
        if (a.F[v]) {
            ofs << "    " << k << " [label=" << k << " shape=doublecircle];\n";
        } else {
            ofs << "    " << k << " [label=" << k << " shape=circle];\n";
        }
    }

    int rows = a.Q.size();
    int cols = a.Σ.size();
    for (int row = 0; row < rows; row++) {
        std::string from_name = find_by_value(a.Q, row);
        for (int col = 0; col < cols; col++) {
            std::string letter = find_by_value(a.Σ, col);
            int to = a.δ[row][col];
            if (to == -1) {
                continue;
            }
            std::string to_name = find_by_value(a.Q, to);
            ofs << "    " << from_name << " -> " << to_name << " [label=\"" << letter << "\"];\n";
        }
    }

    ofs << "}\n";
}

int main() {
    std::vector<bool> important_states;
    FSA a;
    a.s = 0;
    a.Q["0"] = 0;
    important_states.push_back(false);
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream ifs(line);

        std::string plus_or_minus;
        ifs >> plus_or_minus;
        std::vector<std::string> word;
        std::string token;
        while (ifs >> token) {
            if (a.Σ.find(token) == a.Σ.end()) {
                int next_id = a.Σ.size();
                a.Σ[token] = next_id;
                if (a.δ.size() > 0) {
                    for (int i = 0; i < a.δ.size(); i++) {
                        a.δ[i].push_back(0);
                    }
                    assert(a.Σ.size() == a.δ[0].size());
                }
            }

            word.push_back(token);
        }

        if (plus_or_minus == "+") {
            if (a.δ.size() == 0) {
                a.δ.push_back(std::vector<int>(a.Σ.size(), 0));
                a.F.push_back(true);
            }

            int q = a.s;
            for (int i = 0; i < word.size(); i++) {
                int letter = a.Σ[word[i]];
                if (a.δ[q][letter] == 0) {
                    int next_state_id = a.δ.size();
                    a.δ.push_back(std::vector<int>(a.Σ.size(), 0));
                    a.F.push_back(true);
                    important_states.push_back(false);
                    a.δ[q][letter] = next_state_id;
                    a.Q[std::to_string(next_state_id)] = next_state_id;
                }
                q = a.δ[q][letter];
            }

            if (!a.F[q] && important_states[q]) {
                std::cout << "Conflict!\n";
                return 1;
            } else {
                a.F[q] = true;
            }
            important_states[q] = true;
        } else {
            if (a.δ.size() == 0) {
                a.δ.push_back(std::vector<int>(a.Σ.size(), 0));
                a.F.push_back(false);
            }

            int q = a.s;
            for (int i = 0; i < word.size(); i++) {
                int letter = a.Σ[word[i]];
                if (a.δ[q][letter] == 0) {
                    int next_state_id = a.δ.size();
                    a.δ.push_back(std::vector<int>(a.Σ.size(), 0));
                    a.F.push_back(false);
                    important_states.push_back(false);
                    a.δ[q][letter] = next_state_id;
                    a.Q[std::to_string(next_state_id)] = next_state_id;
                }
                q = a.δ[q][letter];
            }

            if (a.F[q] && important_states[q]) {
                std::cout << "Conflict!\n";
                return 1;
            } else {
                a.F[q] = false;
            }
            important_states[q] = true;
        }

        print_fsa_to_dot_file(a, "out_a.dot");
        FSA patched_up_a = patch_up_fsa(a);
        //print_fsa_to_stdout(patched_up_a);
        print_fsa_to_dot_file(patched_up_a, "out.dot");
        system("dot -Tsvg out_a.dot > out_a.svg");
        system("dot -Tsvg out.dot > out.svg");
    }

    a = patch_up_fsa(a);
}
