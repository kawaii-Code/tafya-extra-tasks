#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <vector>
#include <map>
#include <set>

#define debug(x) std::cout << #x << " = " << x << std::endl

// Состояния -- [0 .. |F|]
// Символы алфавита -- [0 .. |Σ|]
struct FSA {
    std::map<std::string, int> Σ;
    std::map<std::string, int> Q;
    std::vector<std::vector<int>> δ;
    std::vector<bool> F; // F[i] == true  <=>  i - допускающее состояние
    int s; // Начальное состояние
};

FSA parse_fsa(const std::string &filename) {
    FSA a;

    std::ifstream ifs(filename);
    std::string line;
    if (std::getline(ifs, line)) {
        std::istringstream alphabet_stream(line);
        std::string token;
        while (alphabet_stream >> token) {
            a.Σ[token] = a.Σ.size();
        }
    }

    int n = a.Σ.size();
    a.F = std::vector<bool>(n);

    int state_count = 0;
    while (std::getline(ifs, line)) {
        std::istringstream line_stream(line);

        bool is_final = false;
        bool is_start = false;

        std::string from;
        line_stream >> from;
        if (from == "*") {
            is_start = true;
            line_stream >> from;
        }
        if (from == "!") {
            is_final = true;
            line_stream >> from;
        }
        if (a.Q.find(from) == a.Q.end()) {
            a.Q[from] = state_count;
            state_count += 1;
            a.δ.push_back(std::vector<int>(n, -1));
        }

        std::string arrow;
        line_stream >> arrow;
        assert(arrow == "->");

        int i_from = a.Q[from];

        for (int i = 0; i < a.Σ.size(); i++) {
            std::string to;
            line_stream >> to;
            if (a.Q.find(to) == a.Q.end()) {
                a.Q[to] = state_count;
                a.δ.push_back(std::vector<int>(n, -1));
                state_count += 1;
            }

            int i_to = a.Q[to];
            a.δ[i_from][i] = i_to;
        }

        if (is_start) {
            a.s = i_from;
        }
        if (is_final) {
            a.F[i_from] = true;
        }
    }

    return a;
}

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
            std::string to_name = find_by_value(a.Q, a.δ[row][col]);
            ofs << "    " << from_name << " -> " << to_name << " [label=\"" << letter << "\"];\n";
        }
    }

    ofs << "}\n";
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
    it++;
    os << *it << "}";

    return os;
}

/*

Hopcroft's algorithm, https://en.wikipedia.org/wiki/DFA_minimization#Hopcroft's_algorithm

P := {F, F \ F}
W := {F, F \ F}

while (W is not empty) do
    choose and remove a set A from W
    for each c in Σ do
        let X be the set of Q for which a transition on c leads to a state in A
        for each set Y in P for which X ∩ Y is nonempty and Y \ X is nonempty do
            replace Y in P by the two sets X ∩ Y and Y \ X
            if Y is in W
                replace Y in W by the same two sets
            else
                if |X ∩ Y| <= |Y \ X|
                    add X ∩ Y to W
                else
                    add Y \ X to W

*/
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

    assert(final_states.size() > 0);
    assert(non_final_states.size() > 0);

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
        debug(A);

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

                debug(X);
                debug(Y);
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

bool fsa_equal(const FSA &a1, const FSA &a2) {
    if (a1.Σ.size() != a1.Σ.size()) {
        return false;
    }
    if (a1.δ.size() != a2.δ.size()) {
        return false;
    }

    for (int i = 0; i < a1.Σ.size(); i++) {
        bool found_equal_col = false;
        for (int j = 0; j < a2.Σ.size(); j++) {
            bool equal = true;
            for (int k = 0; k < a1.δ.size(); k++) {
                if (a1.δ[k][i] != a2.δ[k][j]) {
                    equal = false;
                }
            }

            if (equal) {
                found_equal_col = true;
                break;
            }
        }

        if (!found_equal_col) {
            return false;
        }
    }

    return true;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <file1> <file2>\n";
        return 1;
    }

    FSA a1 = parse_fsa(argv[1]);
    FSA a2 = parse_fsa(argv[2]);

    print_fsa_to_stdout(a1);
    std::cout << "------------\n";
    print_fsa_to_stdout(a2);

    print_fsa_to_dot_file(a1, "a1.dot");
    print_fsa_to_dot_file(a2, "a2.dot");

    std::cout << "\n-----------------------\n\n";

    FSA a1_min = minimize(a1);
    FSA a2_min = minimize(a2);

    print_fsa_to_stdout(a1_min);
    std::cout << "------------\n";
    print_fsa_to_stdout(a2_min);

    print_fsa_to_dot_file(a1_min, "a1_min.dot");
    print_fsa_to_dot_file(a2_min, "a2_min.dot");


    std::cout << "\n";
    if (fsa_equal(a1_min, a2_min)) {
        std::cout << "Equal\n";
    } else {
        std::cout << "Not equal\n";
    }
}
