#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "widget.h"
#include <memory>
#include <vector>

class UIManager{
public:
    void add_widget(std::unique_ptr<Widget> w) {
        widgets.push_back(std::move(w));
    }
    void draw() {
        for (auto& p : widgets) {
            p->draw();
        }
    }
private:
    std::vector<std::unique_ptr<Widget>> widgets;
};
#endif
