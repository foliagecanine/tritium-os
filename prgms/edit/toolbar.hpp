#ifndef EDIT_TOOLBAR_HPP
#define EDIT_TOOLBAR_HPP

#include "graphics.hpp"
#include <string>
#include <cctype>
#include <cstdio>

static const vga_color_t toolbar_menu_color = VGA_BACKGROUND(VGA_COLOR_LIGHT_GREY) | VGA_FOREGROUND(VGA_COLOR_BLACK);
static const vga_color_t toolbar_item_highlight = VGA_BACKGROUND(VGA_COLOR_LIGHT_GREY) | VGA_FOREGROUND(VGA_COLOR_DARK_GREY);

class ToolbarMenuItem {
private:
    std::string name;
    std::string action;
    int highlighted_char;
    unsigned int min_width;

    ToolbarMenuItem* next = nullptr;

public:
    ToolbarMenuItem(std::string name, std::string action, int highlighted_char = -1, unsigned int min_width = 0) 
        : name(name), action(action), highlighted_char(highlighted_char), min_width(min_width) {
        if (min_width < name.length() + 1) {
            this->min_width = name.length() + 1;
        }
    }

    ToolbarMenuItem* get_next() {
        return next;
    }

    void set_next(ToolbarMenuItem* next) {
        this->next = next;
    }

    unsigned int get_min_width() {
        return min_width;
    }

    std::string get_action() {
        return action;
    }

    char get_activation_key() {
        if (highlighted_char >= 0 && highlighted_char < (int)name.length()) {
            return std::tolower(name[highlighted_char]);
        }
        return '\0';
    }

    void render(GraphicsPanel& panel, unsigned int x, unsigned int y, unsigned int width) {
        panel.draw_rect(x, y, x + width, y + 1, toolbar_menu_color, toolbar_menu_color);
        panel.put_text(x, y, name);
        if (highlighted_char >= 0 && highlighted_char < (int)name.length()) {
            panel.set_color(x + highlighted_char, y, toolbar_item_highlight);
        }
    }

    unsigned int get_tree_width() {
        unsigned int tree_min_width = 0;
        
        if (next != nullptr) {
            tree_min_width = next->get_tree_width();
        }

        if (tree_min_width < min_width) {
            tree_min_width = min_width;
        }

        return tree_min_width;
    }

    void render_menuitem_tree(GraphicsPanel& panel, unsigned int x, unsigned int y, unsigned int width) {
        render(panel, x, y, width);
        if (next != nullptr) {
            next->render_menuitem_tree(panel, x, y + 1, width);
        }
    }
};
    

class ToolbarMenu {
private:
    std::string name;
    int highlighted_char;
    bool selected = false;

    ToolbarMenu* next = nullptr;
    
    ToolbarMenuItem* first_item = nullptr;

public:
    ToolbarMenu(std::string name, int highlighted_char = -1) : highlighted_char(highlighted_char) {
        this->name = std::string("\263") + name + std::string("\263");
        
        if (highlighted_char >= 0 && highlighted_char < (int)name.length()) {
            this->highlighted_char = highlighted_char + 1;
        } else {
            this->highlighted_char = -1;
        }
    }

    ~ToolbarMenu() {
        ToolbarMenuItem* item = first_item;
        while (item != nullptr) {
            ToolbarMenuItem* next_item = item->get_next();
            delete item;
            item = next_item;
        }
    }

    unsigned int get_width() {
        return name.length() - 1;
    }

    ToolbarMenuItem* get_first_item() {
        return first_item;
    }

    void set_first_item(ToolbarMenuItem* item) {
        first_item = item;
    }

    void select() {
        selected = true;
    }

    void deselect() {
        selected = false;
    }

    bool is_selected() {
        return selected;
    }

    ToolbarMenu* get_next() {
        return next;
    }

    void set_next(ToolbarMenu* next) {
        this->next = next;
    }

    char get_activation_key() {
        if (highlighted_char >= 0 && highlighted_char < (int)name.length()) {
            return std::tolower(name[highlighted_char]);
        }
        return '\0';
    }

    void render(GraphicsPanel& panel, unsigned int x) {
        panel.draw_rect(x, 0, x + get_width(), 1, toolbar_menu_color, toolbar_menu_color);
        panel.put_text(x, 0, name);
        if (highlighted_char >= 0 && highlighted_char < (int)name.length()) {
            panel.set_color(x + highlighted_char, 0, toolbar_item_highlight);
        }

        if (selected && first_item != nullptr) {
            first_item->render_menuitem_tree(panel, x, 1, first_item->get_tree_width());
        }
    }

    void render_menu_tree(GraphicsPanel& panel, unsigned int x) {
        render(panel, x);
        if (next != nullptr) {
            next->render_menu_tree(panel, x + get_width());
        }
    }
};

#endif
