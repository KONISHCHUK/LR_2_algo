#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace std;

enum class Color { Red, Black };
enum class ServiceType { Web, Database, API, System };

const int SERVICE_TYPE_COUNT = 4;

struct Service {
    int id;
    string name;
    ServiceType type;
    int priority;
    int activeUsers;
};

struct Node {
    Service data;
    Color color;
    Node* left;
    Node* right;
    Node* parent;

    Node(Service value, Color shade)
        : data(std::move(value)), color(shade),
          left(nullptr), right(nullptr), parent(nullptr) {}
};

// Счётчики событий балансировки при вставке (нужны для задания 10).
struct BalanceStats {
    int redUncle = 0;
    int ll = 0;
    int rr = 0;
    int lr = 0;
    int rl = 0;
    int leftRotations = 0;
    int rightRotations = 0;
};

// Показатели, которые собирает обход дерева для пункта меню 7.
struct TreeStats {
    int red = 0;
    int black = 0;
    int leaves = 0;
    int maxDepth = 0;
    long long depthSum = 0;
    int types[SERVICE_TYPE_COUNT] = {};
};

string colorName(Color color) {
    return color == Color::Red ? "RED" : "BLACK";
}

char colorLetter(Color color) {
    return color == Color::Red ? 'R' : 'B';
}

string typeName(ServiceType type) {
    switch (type) {
        case ServiceType::Web: return "Web";
        case ServiceType::Database: return "Database";
        case ServiceType::API: return "API";
        case ServiceType::System: return "System";
    }
    return "?";
}

// Дополняет строку пробелами до нужной ширины. Символы считаются по UTF-8,
// потому что setw считает байты и сбивает колонки на кириллице.
string pad(const string& text, size_t width) {
    size_t length = 0;
    for (char c : text) {
        if ((static_cast<unsigned char>(c) & 0xC0) != 0x80) {
            ++length;
        }
    }
    return text + string(width > length ? width - length : 1, ' ');
}

class RedBlackTree {
    Node* root;
    Node* nil;
    size_t count = 0;
    bool tracing = false;
    bool restored = false;
    BalanceStats balance;

    string label(Node* node) const {
        if (node == nil) {
            return "NIL(B)";
        }
        return to_string(node->data.id) + "(" + colorLetter(node->color) + ")";
    }

    void LeftRotate(Node* x) {
        if (tracing) {
            cout << "Выполнен левый поворот вокруг узла " << x->data.id << ".\n";
        }
        ++balance.leftRotations;
        Node* y = x->right;
        x->right = y->left;
        if (y->left != nil) {
            y->left->parent = x;
        }
        y->parent = x->parent;
        if (x->parent == nil) {
            root = y;
        } else if (x == x->parent->left) {
            x->parent->left = y;
        } else {
            x->parent->right = y;
        }
        y->left = x;
        x->parent = y;
    }

    void RightRotate(Node* y) {
        if (tracing) {
            cout << "Выполнен правый поворот вокруг узла " << y->data.id << ".\n";
        }
        ++balance.rightRotations;
        Node* x = y->left;
        y->left = x->right;
        if (x->right != nil) {
            x->right->parent = y;
        }
        x->parent = y->parent;
        if (y->parent == nil) {
            root = x;
        } else if (y == y->parent->right) {
            y->parent->right = x;
        } else {
            y->parent->left = x;
        }
        x->right = y;
        y->parent = x;
    }

    void traceInsertState(Node* z, Node* uncle) const {
        if (!tracing) {
            return;
        }
        cout << "Родитель: " << label(z->parent) << "\n";
        cout << "Дедушка: " << label(z->parent->parent) << "\n";
        cout << "Дядя: " << label(uncle) << "\n";
        cout << "Обнаружено нарушение: родитель и узел " << z->data.id << " красные.\n";
    }

    void traceRecolor(Node* uncle) const {
        if (tracing) {
            cout << "Действие: перекрашивание родителя и дяди " << uncle->data.id
                 << " в BLACK.\n";
            cout << "Цвет дедушки изменён на RED.\n";
        }
    }

    void traceRotationCase(const char* caseName) const {
        if (tracing) {
            cout << "Чёрный дядя, случай " << caseName << ": поворот и перекрашивание.\n";
        }
    }

    void traceRotationColors(Node* z) const {
        if (tracing) {
            cout << "Цвет узла " << z->parent->data.id << " изменён на BLACK, цвет узла "
                 << z->parent->parent->data.id << " изменён на RED.\n";
        }
    }

    void fixInsert(Node* z) {
        while (z->parent->color == Color::Red) {
            if (z->parent == z->parent->parent->left) {
                Node* uncle = z->parent->parent->right;
                traceInsertState(z, uncle);
                if (uncle->color == Color::Red) {
                    ++balance.redUncle;
                    traceRecolor(uncle);
                    z->parent->color = Color::Black;
                    uncle->color = Color::Black;
                    z->parent->parent->color = Color::Red;
                    z = z->parent->parent;
                } else {
                    if (z == z->parent->right) {
                        ++balance.lr;
                        traceRotationCase("LR");
                        z = z->parent;
                        LeftRotate(z);
                    } else {
                        ++balance.ll;
                        traceRotationCase("LL");
                    }
                    traceRotationColors(z);
                    z->parent->color = Color::Black;
                    z->parent->parent->color = Color::Red;
                    RightRotate(z->parent->parent);
                }
            } else {
                Node* uncle = z->parent->parent->left;
                traceInsertState(z, uncle);
                if (uncle->color == Color::Red) {
                    ++balance.redUncle;
                    traceRecolor(uncle);
                    z->parent->color = Color::Black;
                    uncle->color = Color::Black;
                    z->parent->parent->color = Color::Red;
                    z = z->parent->parent;
                } else {
                    if (z == z->parent->left) {
                        ++balance.rl;
                        traceRotationCase("RL");
                        z = z->parent;
                        RightRotate(z);
                    } else {
                        ++balance.rr;
                        traceRotationCase("RR");
                    }
                    traceRotationColors(z);
                    z->parent->color = Color::Black;
                    z->parent->parent->color = Color::Red;
                    LeftRotate(z->parent->parent);
                }
            }
        }
        if (tracing && root->color == Color::Red) {
            cout << "Корень " << root->data.id << " перекрашен в BLACK.\n";
        }
        root->color = Color::Black;
    }

    Node* minimum(Node* node) const {
        while (node->left != nil) {
            node = node->left;
        }
        return node;
    }

    void transplant(Node* oldNode, Node* newNode) {
        if (oldNode->parent == nil) {
            root = newNode;
        } else if (oldNode == oldNode->parent->left) {
            oldNode->parent->left = newNode;
        } else {
            oldNode->parent->right = newNode;
        }
        newNode->parent = oldNode->parent;
    }

    void traceSibling(Node* x, Node* sibling) const {
        if (tracing) {
            cout << "Дефицит чёрной высоты на месте " << label(x) << ", родитель "
                 << label(x->parent) << ", брат " << label(sibling) << ".\n";
        }
    }

    void traceDeleteCase(const char* text) const {
        if (tracing) {
            cout << text << "\n";
        }
    }

    void fixDelete(Node* x) {
        while (x != root && x->color == Color::Black) {
            if (x == x->parent->left) {
                Node* sibling = x->parent->right;
                traceSibling(x, sibling);
                if (sibling->color == Color::Red) {
                    traceDeleteCase("Случай 1: брат красный, поворот вокруг родителя.");
                    sibling->color = Color::Black;
                    x->parent->color = Color::Red;
                    LeftRotate(x->parent);
                    sibling = x->parent->right;
                }
                if (sibling->left->color == Color::Black
                    && sibling->right->color == Color::Black) {
                    traceDeleteCase("Случай 2: дети брата чёрные, брат перекрашен в RED.");
                    sibling->color = Color::Red;
                    x = x->parent;
                } else {
                    if (sibling->right->color == Color::Black) {
                        traceDeleteCase("Случай 3: дальний ребёнок брата чёрный.");
                        sibling->left->color = Color::Black;
                        sibling->color = Color::Red;
                        RightRotate(sibling);
                        sibling = x->parent->right;
                    }
                    traceDeleteCase("Случай 4: дальний ребёнок брата красный.");
                    sibling->color = x->parent->color;
                    x->parent->color = Color::Black;
                    sibling->right->color = Color::Black;
                    LeftRotate(x->parent);
                    x = root;
                }
            } else {
                Node* sibling = x->parent->left;
                traceSibling(x, sibling);
                if (sibling->color == Color::Red) {
                    traceDeleteCase("Случай 1: брат красный, поворот вокруг родителя.");
                    sibling->color = Color::Black;
                    x->parent->color = Color::Red;
                    RightRotate(x->parent);
                    sibling = x->parent->left;
                }
                if (sibling->right->color == Color::Black
                    && sibling->left->color == Color::Black) {
                    traceDeleteCase("Случай 2: дети брата чёрные, брат перекрашен в RED.");
                    sibling->color = Color::Red;
                    x = x->parent;
                } else {
                    if (sibling->left->color == Color::Black) {
                        traceDeleteCase("Случай 3: дальний ребёнок брата чёрный.");
                        sibling->right->color = Color::Black;
                        sibling->color = Color::Red;
                        LeftRotate(sibling);
                        sibling = x->parent->left;
                    }
                    traceDeleteCase("Случай 4: дальний ребёнок брата красный.");
                    sibling->color = x->parent->color;
                    x->parent->color = Color::Black;
                    sibling->left->color = Color::Black;
                    RightRotate(x->parent);
                    x = root;
                }
            }
        }
        if (tracing && x != nil && x->color == Color::Red) {
            cout << "Узел " << x->data.id << " перекрашен в BLACK.\n";
        }
        x->color = Color::Black;
    }

    void destroy(Node* node) {
        if (node == nil) {
            return;
        }
        destroy(node->left);
        destroy(node->right);
        delete node;
    }

    int heightOf(Node* node) const {
        if (node == nil) {
            return 0;
        }
        return 1 + max(heightOf(node->left), heightOf(node->right));
    }

    void printSideways(Node* node, int depth) const {
        if (node == nil) {
            return;
        }
        printSideways(node->right, depth + 1);
        cout << string(static_cast<size_t>(depth) * 4, ' ') << label(node) << '\n';
        printSideways(node->left, depth + 1);
    }

    void listInOrder(Node* node) const {
        if (node == nil) {
            return;
        }
        listInOrder(node->left);
        const Service& s = node->data;
        cout << pad(to_string(s.id), 6) << pad(s.name, 25) << pad(typeName(s.type), 10)
             << pad(to_string(s.priority), 12) << s.activeUsers << '\n';
        listInOrder(node->right);
    }

    void collect(Node* node, int depth, TreeStats& stats) const {
        if (node == nil) {
            return;
        }
        if (node->color == Color::Red) {
            ++stats.red;
        } else {
            ++stats.black;
        }
        ++stats.types[static_cast<int>(node->data.type)];
        stats.maxDepth = max(stats.maxDepth, depth);
        stats.depthSum += depth;
        if (node->left == nil && node->right == nil) {
            ++stats.leaves;
        }
        collect(node->left, depth + 1, stats);
        collect(node->right, depth + 1, stats);
    }

    // Возвращает чёрную высоту поддерева: NIL считается как один чёрный узел.
    int check(Node* node, long long low, long long high,
              bool& bstOk, bool& redOk, bool& heightOk) const {
        if (node == nil) {
            return 1;
        }
        if (node->data.id <= low || node->data.id >= high) {
            bstOk = false;
        }
        if (node->color == Color::Red
            && (node->left->color == Color::Red || node->right->color == Color::Red)) {
            redOk = false;
        }
        int leftHeight = check(node->left, low, node->data.id, bstOk, redOk, heightOk);
        int rightHeight = check(node->right, node->data.id, high, bstOk, redOk, heightOk);
        if (leftHeight != rightHeight) {
            heightOk = false;
        }
        return leftHeight + (node->color == Color::Black ? 1 : 0);
    }

    // order: 0 - прямой обход, 1 - симметричный, 2 - обратный.
    void traversal(Node* node, int order, vector<int>& keys) const {
        if (node == nil) {
            return;
        }
        if (order == 0) {
            cout << label(node) << ' ';
        }
        traversal(node->left, order, keys);
        if (order == 1) {
            cout << label(node) << ' ';
            keys.push_back(node->data.id);
        }
        traversal(node->right, order, keys);
        if (order == 2) {
            cout << label(node) << ' ';
        }
    }

public:
    RedBlackTree() {
        nil = new Node({0, "", ServiceType::System, 0, 0}, Color::Black);
        nil->left = nil;
        nil->right = nil;
        nil->parent = nil;
        root = nil;
    }

    ~RedBlackTree() {
        destroy(root);
        delete nil;
    }

    RedBlackTree(const RedBlackTree&) = delete;
    RedBlackTree& operator=(const RedBlackTree&) = delete;

    size_t size() const { return count; }
    int height() const { return heightOf(root); }
    bool wasRestored() const { return restored; }
    const BalanceStats& balanceStats() const { return balance; }
    void setTracing(bool enabled) { tracing = enabled; }

    Node* search(int key) const {
        vector<int> path;
        return search(key, path);
    }

    // Записывает в path все пройденные ключи. Число сравнений равно path.size().
    Node* search(int key, vector<int>& path) const {
        Node* node = root;
        while (node != nil) {
            path.push_back(node->data.id);
            if (key == node->data.id) {
                return node;
            }
            node = key < node->data.id ? node->left : node->right;
        }
        return nullptr;
    }

    bool insert(const Service& value) {
        Node* parent = nil;
        Node* current = root;
        while (current != nil) {
            parent = current;
            if (value.id == current->data.id) {
                return false;
            }
            current = value.id < current->data.id ? current->left : current->right;
        }
        if (tracing) {
            cout << "Вставка ключа: " << value.id << "\n";
        }
        Node* node = new Node(value, Color::Red);
        node->left = nil;
        node->right = nil;
        node->parent = parent;
        if (parent == nil) {
            root = node;
        } else if (value.id < parent->data.id) {
            parent->left = node;
        } else {
            parent->right = node;
        }
        ++count;
        if (tracing && parent != nil && parent->color == Color::Black) {
            cout << "Родитель: " << label(parent) << "\nРодитель чёрный, нарушений нет.\n";
        }
        fixInsert(node);
        return true;
    }

    bool remove(int id) {
        Node* z = search(id);
        if (z == nullptr) {
            return false;
        }
        if (tracing) {
            cout << "Удаление ключа: " << id << "\nУзел: " << label(z)
                 << ", левый потомок " << label(z->left)
                 << ", правый потомок " << label(z->right) << "\n";
        }
        Node* y = z;
        Color removedColor = y->color;
        Node* x;
        if (z->left == nil) {
            x = z->right;
            transplant(z, z->right);
        } else if (z->right == nil) {
            x = z->left;
            transplant(z, z->left);
        } else {
            // Два потомка: на место z встаёт его преемник y (минимум правого поддерева).
            y = minimum(z->right);
            removedColor = y->color;
            x = y->right;
            if (tracing) {
                cout << "Два потомка: преемник " << label(y) << " занимает место узла.\n";
            }
            if (y->parent == z) {
                x->parent = y;
            } else {
                transplant(y, y->right);
                y->right = z->right;
                y->right->parent = y;
            }
            transplant(z, y);
            y->left = z->left;
            y->left->parent = y;
            y->color = z->color;
        }
        delete z;
        --count;
        restored = removedColor == Color::Black;
        if (restored) {
            fixDelete(x);
        } else if (tracing) {
            cout << "Вырезан красный узел, чёрная высота не изменилась.\n";
        }
        return true;
    }

    void update(int id, const Service& value) {
        Node* node = search(id);
        if (node != nullptr) {
            node->data.name = value.name;
            node->data.type = value.type;
            node->data.priority = value.priority;
            node->data.activeUsers = value.activeUsers;
        }
    }

    void print() const {
        if (root == nil) {
            cout << "Дерево пусто.\n";
        } else {
            printSideways(root, 0);
        }
    }

    void list() const {
        cout << pad("ID", 6) << pad("Название", 25) << pad("Тип", 10)
             << pad("Приоритет", 12) << "Пользователи\n";
        listInOrder(root);
    }

    void printTraversals() const {
        const char* labels[] = {"Прямой обход", "Симметричный обход", "Обратный обход"};
        vector<int> keys;
        for (int order = 0; order < 3; ++order) {
            cout << labels[order] << ": ";
            traversal(root, order, keys);
            cout << '\n';
        }
        cout << "Ключи симметричного обхода отсортированы: "
             << (is_sorted(keys.begin(), keys.end()) ? "да" : "нет") << '\n';
    }

    void statistics() const {
        TreeStats stats;
        collect(root, 0, stats);
        double averageDepth = 0.0;
        if (count > 0) {
            averageDepth = static_cast<double>(stats.depthSum) / static_cast<double>(count);
        }
        cout << "Количество сервисов: " << count << "\n";
        cout << "Высота дерева: " << height() << "\n";
        cout << "Максимальная глубина: " << stats.maxDepth << "\n";
        cout << "Средняя глубина: " << fixed << setprecision(2) << averageDepth << "\n";
        cout << "Красных узлов: " << stats.red << "\n";
        cout << "Чёрных узлов: " << stats.black << "\n";
        cout << "Листьев: " << stats.leaves << "\n";
        for (int i = 0; i < SERVICE_TYPE_COUNT; ++i) {
            cout << typeName(static_cast<ServiceType>(i)) << ": " << stats.types[i] << '\n';
        }
    }

    bool validate(bool show = true) const {
        bool bst = true;
        bool red = true;
        bool blackHeight = true;
        check(root, numeric_limits<long long>::min(), numeric_limits<long long>::max(),
              bst, red, blackHeight);
        bool rootOk = root == nil || root->color == Color::Black;
        if (show) {
            cout << "BST property: "
                 << (bst ? "OK" : "ERROR (нарушен порядок ключей)") << "\n";
            cout << "Root color: "
                 << (rootOk ? "OK" : "ERROR (корень не чёрный)") << "\n";
            cout << "Red-red violation: "
                 << (red ? "not found" : "found (красный узел с красным потомком)") << "\n";
            cout << "Black height: "
                 << (blackHeight ? "OK" : "ERROR (разное число чёрных узлов)") << "\n";
        }
        return bst && rootOk && red && blackHeight;
    }

    // Намеренная порча дерева: нужна только для проверки validate в задании 9.
    void damageColor(int id) {
        Node* node = search(id);
        if (node != nullptr) {
            node->color = node->color == Color::Red ? Color::Black : Color::Red;
        }
    }

    void damageKey(int id, int newId) {
        Node* node = search(id);
        if (node != nullptr) {
            node->data.id = newId;
        }
    }
};

// Обычное двоичное дерево поиска без балансировки: нужно для заданий 1, 13 и 14.
class PlainBST {
    struct Item {
        int key;
        Item* left = nullptr;
        Item* right = nullptr;
        explicit Item(int k) : key(k) {}
    };

    Item* root = nullptr;
    int count = 0;

    void destroy(Item* node) {
        if (node == nullptr) {
            return;
        }
        destroy(node->left);
        destroy(node->right);
        delete node;
    }

    int heightOf(Item* node) const {
        if (node == nullptr) {
            return 0;
        }
        return 1 + max(heightOf(node->left), heightOf(node->right));
    }

    void printSideways(Item* node, int depth) const {
        if (node == nullptr) {
            return;
        }
        printSideways(node->right, depth + 1);
        cout << string(static_cast<size_t>(depth) * 4, ' ') << node->key << '\n';
        printSideways(node->left, depth + 1);
    }

public:
    PlainBST() = default;
    PlainBST(const PlainBST&) = delete;
    PlainBST& operator=(const PlainBST&) = delete;

    ~PlainBST() { destroy(root); }

    void insert(int key) {
        if (root == nullptr) {
            root = new Item(key);
            count = 1;
            return;
        }
        Item* current = root;
        while (key != current->key) {
            if (key < current->key) {
                if (current->left == nullptr) {
                    current->left = new Item(key);
                    ++count;
                    return;
                }
                current = current->left;
            } else {
                if (current->right == nullptr) {
                    current->right = new Item(key);
                    ++count;
                    return;
                }
                current = current->right;
            }
        }
    }

    int height() const { return heightOf(root); }
    int size() const { return count; }

    int comparisons(int key) const {
        int result = 0;
        Item* current = root;
        while (current != nullptr) {
            ++result;
            if (key == current->key) {
                break;
            }
            current = key < current->key ? current->left : current->right;
        }
        return result;
    }

    void print() const { printSideways(root, 0); }
};

// Исходный набор: порядок вставки даёт перекрашивания и случай LR.
const Service INITIAL_SERVICES[] = {
    {45, "Frontend", ServiceType::Web, 3, 250},
    {20, "Database Core", ServiceType::Database, 5, 600},
    {70, "API Gateway", ServiceType::API, 4, 420},
    {10, "Log Service", ServiceType::System, 2, 80},
    {30, "Auth API", ServiceType::API, 5, 380},
    {60, "Analytics", ServiceType::Web, 3, 210},
    {90, "Backup", ServiceType::System, 4, 45},
    {15, "Profile API", ServiceType::API, 3, 160},
    {25, "Payments DB", ServiceType::Database, 5, 500},
    {35, "Search", ServiceType::Web, 4, 310},
    {55, "Metrics", ServiceType::System, 2, 95},
    {65, "Reports", ServiceType::Web, 3, 175},
    {75, "Queue API", ServiceType::API, 4, 290},
    {85, "Archive DB", ServiceType::Database, 3, 60},
    {95, "Notifier", ServiceType::System, 2, 130},
    {5, "Admin UI", ServiceType::Web, 4, 70},
    {40, "Session API", ServiceType::API, 5, 340},
    {80, "Cache", ServiceType::Database, 4, 450}
};

// Бросается при конце ввода (Ctrl+D): программа завершается, а не зацикливается.
struct InputEnded {};

string readLine(const string& prompt) {
    cout << prompt;
    string value;
    if (!getline(cin, value)) {
        throw InputEnded{};
    }
    return value;
}

int readInt(const string& prompt) {
    while (true) {
        istringstream input(readLine(prompt));
        int value;
        char extra;
        if (input >> value && !(input >> extra)) {
            return value;
        }
        cout << "Введите целое число.\n";
    }
}

ServiceType readType() {
    while (true) {
        int value = readInt("Тип (1-Web, 2-Database, 3-API, 4-System): ");
        if (value >= 1 && value <= SERVICE_TYPE_COUNT) {
            return static_cast<ServiceType>(value - 1);
        }
        cout << "Введите число от 1 до 4.\n";
    }
}

void printService(const Service& s) {
    cout << "ID: " << s.id << "\n";
    cout << "Название: " << s.name << "\n";
    cout << "Тип: " << typeName(s.type) << "\n";
    cout << "Приоритет: " << s.priority << "\n";
    cout << "Активных пользователей: " << s.activeUsers << "\n";
}

void reportValidation(const RedBlackTree& tree) {
    if (tree.validate(false)) {
        cout << "RBT validation: OK\n";
    } else {
        cout << "RBT validation: ERROR\n";
        tree.validate(true);
    }
}

void printSearch(const RedBlackTree& tree, int id) {
    vector<int> path;
    Node* node = tree.search(id, path);
    cout << "Путь поиска: ";
    for (size_t i = 0; i < path.size(); ++i) {
        cout << (i > 0 ? " -> " : "") << path[i];
    }
    if (node == nullptr) {
        cout << (path.empty() ? "NIL" : " -> NIL");
    }
    cout << "\n";
    cout << (node != nullptr ? "Элемент найден.\n" : "Элемент не найден.\n");
    cout << "Количество сравнений: " << path.size() << "\n";
    if (node != nullptr) {
        printService(node->data);
        cout << "Глубина: " << path.size() - 1 << "\n";
        cout << "Цвет узла: " << colorName(node->color) << "\n";
    }
}

void addService(RedBlackTree& tree) {
    Service s;
    s.id = readInt("Введите ID: ");
    if (tree.search(s.id) != nullptr) {
        cout << "Ошибка: сервис с ID " << s.id << " уже существует.\n";
        return;
    }
    s.name = readLine("Название: ");
    s.type = readType();
    s.priority = readInt("Приоритет: ");
    s.activeUsers = readInt("Активных пользователей: ");
    tree.insert(s);
    cout << "Сервис успешно добавлен.\n";
    reportValidation(tree);
}

void removeService(RedBlackTree& tree, int id) {
    if (!tree.remove(id)) {
        cout << "Сервис не найден.\n";
        return;
    }
    cout << "Удалён сервис с ID " << id << ".\n";
    if (tree.wasRestored()) {
        cout << "Выполнено восстановление структуры.\n";
    } else {
        cout << "Восстановление не потребовалось.\n";
    }
    reportValidation(tree);
}

void editService(RedBlackTree& tree) {
    int id = readInt("Введите ID сервиса: ");
    if (tree.search(id) == nullptr) {
        cout << "Сервис не найден.\n";
        return;
    }
    Service s;
    s.id = id;
    s.name = readLine("Новое название: ");
    s.type = readType();
    s.priority = readInt("Новый приоритет: ");
    s.activeUsers = readInt("Новое количество пользователей: ");
    tree.update(id, s);
    cout << "Данные обновлены.\n";
}

void checkTree(const RedBlackTree& tree) {
    cout << "Проверка структуры...\n\n";
    bool correct = tree.validate();
    cout << (correct ? "\nКрасно-чёрное дерево корректно.\n" : "\nОбнаружено нарушение.\n");
}

void loadInitial(RedBlackTree& tree) {
    for (const Service& service : INITIAL_SERVICES) {
        tree.insert(service);
    }
}

void printKeys(const vector<int>& keys) {
    for (size_t i = 0; i < keys.size(); ++i) {
        cout << (i > 0 ? ", " : "") << keys[i];
    }
    cout << "\n";
}

void studyPlainTree(const vector<int>& keys) {
    PlainBST tree;
    for (int key : keys) {
        tree.insert(key);
    }
    cout << "Порядок вставки: ";
    printKeys(keys);
    tree.print();
    cout << "Количество узлов: " << tree.size() << "\n";
    cout << "Высота: " << tree.height() << "\n";
    cout << "Сравнений при поиске минимального элемента (10): "
         << tree.comparisons(10) << "\n";
    cout << "Сравнений при поиске максимального элемента (90): "
         << tree.comparisons(90) << "\n\n";
}

void task1() {
    cout << "=== Задание 1. Обычное BST ===\n";
    studyPlainTree({10, 20, 30, 40, 50, 60, 70, 80, 90});
    studyPlainTree({50, 20, 70, 10, 30, 60, 80, 40, 90});
}

void studyBalanceCase(const string& caseName, int first, int second, int third) {
    cout << "--- Случай " << caseName << ": ключи "
         << first << ", " << second << ", " << third << " ---\n";
    RedBlackTree tree;
    tree.insert({first, "", ServiceType::Web, 0, 0});
    tree.insert({second, "", ServiceType::Web, 0, 0});
    cout << "Дерево до вставки ключа " << third << ":\n";
    tree.print();
    tree.setTracing(true);
    tree.insert({third, "", ServiceType::Web, 0, 0});
    tree.setTracing(false);
    cout << "Дерево после восстановления:\n";
    tree.print();
    reportValidation(tree);
    cout << "\n";
}

void task5() {
    cout << "=== Задание 5. Случаи балансировки ===\n";
    studyBalanceCase("LL", 30, 20, 10);
    studyBalanceCase("RR", 10, 20, 30);
    studyBalanceCase("LR", 30, 10, 20);
    studyBalanceCase("RL", 10, 30, 20);
}

void task8() {
    cout << "=== Задание 8. Поиск ===\n";
    RedBlackTree tree;
    loadInitial(tree);
    for (int id : {45, 5, 95, 37, 100}) {
        cout << "Искомый ключ: " << id << "\n";
        printSearch(tree, id);
        cout << "\n";
    }
}

void task9() {
    cout << "=== Задание 9. Обнаружение нарушений ===\n";
    {
        RedBlackTree tree;
        loadInitial(tree);
        cout << "Корень 45 перекрашен в красный:\n";
        tree.damageColor(45);
        tree.validate();
    }
    {
        RedBlackTree tree;
        loadInitial(tree);
        cout << "\nЧёрный узел 20 перекрашен в красный:\n";
        tree.damageColor(20);
        tree.validate();
    }
    {
        RedBlackTree tree;
        loadInitial(tree);
        cout << "\nКлюч 5 заменён на 100 без перестроения связей:\n";
        tree.damageKey(5, 100);
        tree.validate();
    }
    cout << "\n";
}

void task11() {
    cout << "=== Задание 11. Удаление ===\n";
    RedBlackTree tree;
    loadInitial(tree);
    tree.setTracing(true);
    const char* titles[] = {
        "лист (красный)", "узел с одним потомком", "узел с двумя потомками", "лист (чёрный)"
    };
    int ids[] = {5, 10, 70, 25};
    for (int i = 0; i < 4; ++i) {
        cout << "--- Delete " << ids[i] << ": " << titles[i] << " ---\n";
        removeService(tree, ids[i]);
        tree.print();
        cout << "\n";
    }
}

vector<int> comparisons(const PlainBST& tree, const vector<int>& keys) {
    vector<int> result;
    for (int key : keys) {
        result.push_back(tree.comparisons(key));
    }
    return result;
}

vector<int> comparisons(const RedBlackTree& tree, const vector<int>& keys) {
    vector<int> result;
    for (int key : keys) {
        vector<int> path;
        tree.search(key, path);
        result.push_back(static_cast<int>(path.size()));
    }
    return result;
}

double average(const vector<int>& values) {
    long long total = 0;
    for (int value : values) {
        total += value;
    }
    return static_cast<double>(total) / static_cast<double>(values.size());
}

int maximum(const vector<int>& values) {
    return *max_element(values.begin(), values.end());
}

void buildPair(const vector<int>& keys, PlainBST& bst, RedBlackTree& rbt) {
    for (int key : keys) {
        bst.insert(key);
        rbt.insert({key, "", ServiceType::Web, 0, 0});
    }
}

void task13() {
    cout << "=== Задание 13. BST и красно-чёрное дерево на одном наборе ===\n";
    vector<int> keys;
    for (const Service& service : INITIAL_SERVICES) {
        keys.push_back(service.id);
    }
    PlainBST bst;
    RedBlackTree rbt;
    buildPair(keys, bst, rbt);
    const vector<int> selected = {45, 20, 70, 5, 95};
    vector<int> bstSelected = comparisons(bst, selected);
    vector<int> rbtSelected = comparisons(rbt, selected);
    vector<int> bstAll = comparisons(bst, keys);
    vector<int> rbtAll = comparisons(rbt, keys);
    cout << "Ключ  BST  RBT\n";
    for (size_t i = 0; i < selected.size(); ++i) {
        cout << pad(to_string(selected[i]), 6) << pad(to_string(bstSelected[i]), 5)
             << rbtSelected[i] << "\n";
    }
    cout << fixed << setprecision(2);
    cout << "Количество узлов: " << bst.size() << " и " << rbt.size() << "\n";
    cout << "Высота: " << bst.height() << " и " << rbt.height() << "\n";
    cout << "Максимальная глубина: " << bst.height() - 1 << " и " << rbt.height() - 1
         << "\n";
    cout << "Среднее сравнений по 5 ключам: " << average(bstSelected) << " и "
         << average(rbtSelected) << "\n";
    cout << "Среднее сравнений по всем ключам: " << average(bstAll) << " и "
         << average(rbtAll) << "\n";
    cout << "Максимум сравнений: " << maximum(bstAll) << " и " << maximum(rbtAll) << "\n\n";
}

void studyOrder(const string& title, const vector<int>& keys) {
    PlainBST bst;
    RedBlackTree rbt;
    buildPair(keys, bst, rbt);
    cout << title << ":\n  ";
    printKeys(keys);
    cout << "  Высота BST и RBT: " << bst.height() << " и " << rbt.height() << "\n";
    vector<int> bstAll = comparisons(bst, keys);
    vector<int> rbtAll = comparisons(rbt, keys);
    cout << fixed << setprecision(2);
    cout << "  Среднее сравнений: " << average(bstAll) << " и " << average(rbtAll) << "\n";
    cout << "  Максимум сравнений: " << maximum(bstAll) << " и " << maximum(rbtAll) << "\n";
}

void task14() {
    cout << "=== Задание 14. Порядок входных данных ===\n";
    studyOrder("Массив A, возрастание",
               {5, 10, 15, 20, 25, 30, 35, 40, 45, 55, 60, 65, 70, 75, 80, 85, 90, 95});
    studyOrder("Массив B, убывание",
               {95, 90, 85, 80, 75, 70, 65, 60, 55, 45, 40, 35, 30, 25, 20, 15, 10, 5});
    studyOrder("Массив C, смешанный",
               {45, 20, 70, 10, 30, 60, 90, 15, 25, 35, 55, 65, 75, 85, 95, 5, 40, 80});
}

void runExperiments() {
    task1();
    task5();
    task8();
    task9();
    task11();
    task13();
    task14();
}

void printLoadSummary(const RedBlackTree& tree) {
    const BalanceStats& b = tree.balanceStats();
    cout << "Загружено сервисов: " << tree.size() << "\n";
    cout << "Красный дядя (перекрашивание): " << b.redUncle << "\n";
    cout << "Случаи LL: " << b.ll << ", RR: " << b.rr
         << ", LR: " << b.lr << ", RL: " << b.rl << "\n";
    cout << "Левых поворотов: " << b.leftRotations
         << ", правых поворотов: " << b.rightRotations << "\n";
}

void printMenu() {
    cout << "\n1. Добавить сервис\n";
    cout << "2. Найти сервис\n";
    cout << "3. Удалить сервис\n";
    cout << "4. Изменить данные сервиса\n";
    cout << "5. Вывести все сервисы\n";
    cout << "6. Вывести структуру красно-чёрного дерева\n";
    cout << "7. Показать статистику\n";
    cout << "8. Проверить корректность дерева\n";
    cout << "9. Завершить работу\n";
    cout << "10. Показать обходы дерева\n";
    cout << "11. Эксперименты (задания 1, 5, 8, 9, 11, 13, 14)\n";
}

void runMenu(RedBlackTree& tree) {
    while (true) {
        printMenu();
        int choice = readInt("Выберите пункт: ");
        switch (choice) {
            case 1: addService(tree); break;
            case 2: printSearch(tree, readInt("Введите ID: ")); break;
            case 3: removeService(tree, readInt("Введите ID: ")); break;
            case 4: editService(tree); break;
            case 5: tree.list(); break;
            case 6: tree.print(); break;
            case 7: tree.statistics(); break;
            case 8: checkTree(tree); break;
            case 9: return;
            case 10: tree.printTraversals(); break;
            case 11: runExperiments(); break;
            default: cout << "Нет такого пункта меню.\n";
        }
    }
}

int main() {
    RedBlackTree tree;
    tree.setTracing(true);
    for (const Service& service : INITIAL_SERVICES) {
        tree.insert(service);
        cout << "Insert " << service.id << "\n";
        reportValidation(tree);
        cout << "\n";
    }
    printLoadSummary(tree);
    try {
        runMenu(tree);
    } catch (const InputEnded&) {
        cout << "\nВвод завершён.\n";
    }
    return 0;
}