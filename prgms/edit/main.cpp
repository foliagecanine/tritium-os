#include "graphics.hpp"
#include "toolbar.hpp"
#include <string>
#include <cstdio>
#include <cunixfile>
#include <cgui>
#include <ctty>

using namespace std;

class Editor {
private:
    GraphicsPanel graphics;
    string filename;
    string content;
    string cwd;
    unsigned int cursor_x = 0;
    unsigned int cursor_y = 1;
    unsigned int desired_x = 0;
    unsigned int scroll_x = 0;
    unsigned int scroll_y = 0;
    bool line_end = false;
    bool modified = false;

    const unsigned int screen_width = 80;
    const unsigned int screen_height = 25;

    ToolbarMenu *first_toolbar_menu = nullptr;
    ToolbarMenu *menu_selected = nullptr;
    bool alt_pressed = false;
    bool shift_pressed = false;
    bool ctrl_pressed = false;

    string *lines = new string[1];
    unsigned int line_count = 1;

    struct Selection {
        unsigned int start_x;
        unsigned int start_y;
        unsigned int end_x;
        unsigned int end_y;
        bool active = false;
    } selection;

    struct Clipboard {
        string data;
    } clipboard;

public:
    Editor(string cwd, string filename = "") : cwd(cwd), filename(filename) {
        ToolbarMenu *file_menu = new ToolbarMenu("File", 0);
        ToolbarMenuItem *new_item = new ToolbarMenuItem("New", "new", 0, 10);
        ToolbarMenuItem *open_item = new ToolbarMenuItem("Open", "open", 0);
        ToolbarMenuItem *save_item = new ToolbarMenuItem("Save", "save", 0);
        ToolbarMenuItem *saveas_item = new ToolbarMenuItem("Save As", "saveas", 5);
        ToolbarMenuItem *exit_item = new ToolbarMenuItem("Exit", "exit", 1);
        new_item->set_next(open_item);
        open_item->set_next(save_item);
        save_item->set_next(saveas_item);
        saveas_item->set_next(exit_item);
        file_menu->set_first_item(new_item);

        ToolbarMenu *edit_menu = new ToolbarMenu("Edit", 0);
        // ToolbarMenuItem *undo_item = new ToolbarMenuItem("Undo", "undo", 0, 10);
        // ToolbarMenuItem *redo_item = new ToolbarMenuItem("Redo", "redo", 0);
        ToolbarMenuItem *cut_item = new ToolbarMenuItem("Cut", "cut", 2);
        ToolbarMenuItem *copy_item = new ToolbarMenuItem("Copy", "copy", 0);
        ToolbarMenuItem *paste_item = new ToolbarMenuItem("Paste", "paste", 0);
        // undo_item->set_next(redo_item);
        // redo_item->set_next(cut_item);
        cut_item->set_next(copy_item);
        copy_item->set_next(paste_item);
        // edit_menu->set_first_item(undo_item);
        edit_menu->set_first_item(cut_item);
        file_menu->set_next(edit_menu);

        ToolbarMenu *help_menu = new ToolbarMenu("Help", 0);
        ToolbarMenuItem *about_item = new ToolbarMenuItem("About", "about", 0, 10);
        help_menu->set_first_item(about_item);
        edit_menu->set_next(help_menu);

        first_toolbar_menu = file_menu;

        // Load file if provided
        if (!filename.empty()) {
            FILE *fp = fopen(filename.c_str(), "r");
            if (fp != nullptr) {
                content.clear();
                char buffer[1024];
                size_t bytes_read;
                while ((bytes_read = fread(buffer, 1, sizeof(buffer) - 1, fp)) > 0) {
                    buffer[bytes_read] = '\0';
                    content += string(buffer);
                }
                fclose(fp);
            }

            parse_lines();
        }
    }

    void draw_toolbar(bool hidden = true) {
        graphics.set_buffer(true);
        graphics.draw_rect(0, 0, screen_width, 1, 0x70, 0x70);

        graphics.put_text_centered(0, 0, screen_width, "TritiumOS Text Editor");

        if (!hidden) {
            first_toolbar_menu->render_menu_tree(graphics, 0);
        }
        
        graphics.blit();
        graphics.set_buffer(false);
    }

    void draw_editor_area() {
        graphics.set_buffer(true);
        graphics.draw_rect(0, 1, screen_width, screen_height - 1, 0xF0, 0xF0);
    
        int lines_to_draw = screen_height - 2;
        if (line_count < lines_to_draw) {
            lines_to_draw = line_count;
        }

        for (int i = 0; i < lines_to_draw; i++) {
            unsigned int line_index = i + scroll_y;
            if (line_index < line_count) {
                string line = lines[line_index];

                string visible_line;
                
                if (line.length() < scroll_x) {
                    visible_line = "";
                } else if (line.length() < scroll_x + screen_width) {
                    visible_line = line.substr(scroll_x, line.length() - scroll_x);
                } else {
                    visible_line = line.substr(scroll_x, screen_width);
                }

                graphics.put_text(0, i + 1, visible_line);

                if (selection.active) {
                    unsigned int sel_start_y = selection.start_y - 1;
                    unsigned int sel_end_y = selection.end_y - 1;
                    unsigned int sel_start_x = selection.start_x;
                    unsigned int sel_end_x = selection.end_x;

                    if (sel_start_y > sel_end_y || (sel_start_y == sel_end_y && sel_start_x > sel_end_x)) {
                        swap(sel_start_y, sel_end_y);
                        swap(sel_start_x, sel_end_x);
                    }

                    if (line_index >= sel_start_y && line_index <= sel_end_y) {
                        unsigned int highlight_start_x = 0;
                        unsigned int highlight_end_x = visible_line.length();

                        if (line_index == sel_start_y) {
                            if (sel_start_x > scroll_x) {
                                highlight_start_x = sel_start_x - scroll_x;
                            } else {
                                highlight_start_x = 0;
                            }
                        }

                        if (line_index == sel_end_y) {
                            if (sel_end_x > scroll_x) {
                                highlight_end_x = sel_end_x - scroll_x;
                            } else {
                                highlight_end_x = 0;
                            }
                        }

                        for (unsigned int x = highlight_start_x; x < highlight_end_x && x < screen_width; x++) {
                            graphics.set_color(x, i + 1, 0xB0);
                        }
                    }
                }
            }
        }
        graphics.blit();
        graphics.set_buffer(false);
    }

    void draw_statusbar() {
        graphics.set_buffer(true);
        graphics.draw_rect(0, screen_height - 1, screen_width, screen_height, 0x70, 0x70);

        graphics.put_text(0, screen_height - 1, "\263");

        // Display filename and modified status
        string short_filename;
        if (filename.empty()) {
            short_filename = "(Untitled)";
        } else {
            short_filename = filename;
            size_t last_slash = filename.find_last_of('/');
            if (last_slash != (size_t)-1) {
                short_filename = filename.substr(last_slash + 1, filename.length() - last_slash - 1);
            }
        }

        if (modified) {
            short_filename.append("*");
        }

        graphics.put_text(1, screen_height - 1, short_filename);

        char pos_buffer[50];

        // Calculate the number of selected characters
        unsigned int selected_chars = 0;
        if (selection.active) {
            unsigned int sel_start_y = selection.start_y;
            unsigned int sel_end_y = selection.end_y;
            unsigned int sel_start_x = selection.start_x;
            unsigned int sel_end_x = selection.end_x;

            if (sel_start_y > sel_end_y || (sel_start_y == sel_end_y && sel_start_x > sel_end_x)) {
                swap(sel_start_y, sel_end_y);
                swap(sel_start_x, sel_end_x);
            }

            unsigned int selected_chars = 0;
            if (sel_start_y == sel_end_y) {
                selected_chars = sel_end_x - sel_start_x;
            } else {
                // Count characters from start line
                selected_chars += lines[sel_start_y - 1].length() - sel_start_x;
                // Count full lines in between
                for (unsigned int l = sel_start_y; l < sel_end_y - 1; l++) {
                    selected_chars += lines[l].length();
                }
                // Count characters from end line
                selected_chars += sel_end_x;
            }
        }
        
        // Display cursor position and selected characters
        if (selected_chars == 0) {
            snprintf(pos_buffer, sizeof(pos_buffer), "\263 %u:%u", cursor_y + scroll_y, cursor_x + scroll_x + 1);
        } else {
            snprintf(pos_buffer, sizeof(pos_buffer), "\263 %u:%u (%u selected)", cursor_y + scroll_y, cursor_x + scroll_x + 1, selected_chars);
        }

        graphics.put_text(18, screen_height - 1, pos_buffer);

        // Display menu hint
        string menu_text = "\263Alt: Menu\263";
        graphics.put_text(screen_width - menu_text.length(), screen_height - 1, menu_text);
        graphics.blit();
        graphics.set_buffer(false);
    }

    void redraw() {
        draw_toolbar();
        draw_editor_area();
        draw_statusbar();
    }

    void parse_lines() {
        if (lines != nullptr) {
            delete[] lines;
            lines = nullptr;
        }

        line_count = 1;
        for (size_t i = 0; i < content.length(); i++) {
            if (content[i] == '\n') {
                line_count++;
            }
        }

        lines = new string[line_count];
        unsigned int current_line = 0;
        size_t line_start = 0;

        for (size_t i = 0; i < content.length(); i++) {
            if (content[i] == '\n') {
                lines[current_line] = content.substr(line_start, i - line_start);
                current_line++;
                line_start = i + 1;
            }
        }

        if (line_start < content.length()) {
            lines[current_line] = content.substr(line_start, content.length() - line_start);
        }
    }

    void resolve_content() {
        content.clear();
        for (unsigned int i = 0; i < line_count; i++) {
            content += lines[i];
            if (i < line_count - 1) {
                content += "\n";
            }
        }
    }

    void clipboard_copy() {
        if (selection.active) {
            unsigned int sel_start_y = selection.start_y;
            unsigned int sel_end_y = selection.end_y;
            unsigned int sel_start_x = selection.start_x;
            unsigned int sel_end_x = selection.end_x;

            if (sel_start_y > sel_end_y || (sel_start_y == sel_end_y && sel_start_x > sel_end_x)) {
                swap(sel_start_y, sel_end_y);
                swap(sel_start_x, sel_end_x);
            }

            clipboard.data.clear();
            if (sel_start_y == sel_end_y) {
                clipboard.data = lines[sel_start_y - 1].substr(sel_start_x, sel_end_x - sel_start_x);
            } else {
                // Copy from start line
                clipboard.data = lines[sel_start_y - 1].substr(sel_start_x) + "\n";
                
                // Copy full lines in between
                for (unsigned int l = sel_start_y; l < sel_end_y - 1; l++) {
                    clipboard.data += lines[l] + "\n";
                }
                // Copy from end line
                clipboard.data += lines[sel_end_y - 1].substr(0, sel_end_x);
            }
        }
    }

    void clipboard_paste() {
        if (!clipboard.data.empty()) {
            string &current_line = lines[cursor_y + scroll_y - 1];
            
            // Split clipboard data into lines
            size_t pos = 0;
            size_t next_pos;
            string before_cursor = current_line.substr(0, cursor_x + scroll_x);
            string after_cursor = current_line.substr(cursor_x + scroll_x);

            bool first_line = true;

            while ((next_pos = clipboard.data.find(string("\n"), pos)) != string::npos) {
                string clip_line = clipboard.data.substr(pos, next_pos - pos);
                if (first_line) {
                    current_line = before_cursor + clip_line;
                    first_line = false;
                } else {
                    // Insert new line
                    line_count++;
                    string *new_lines = new string[line_count];
                    for (unsigned int i = 0; i < line_count - 1; i++) {
                        new_lines[i] = lines[i];
                    }
                    new_lines[cursor_y + scroll_y] = clip_line;
                    for (unsigned int i = cursor_y + scroll_y + 1; i < line_count; i++) {
                        new_lines[i] = lines[i - 1];
                    }
                    delete[] lines;
                    lines = new_lines;
                }
                pos = next_pos + 1;
            }

            // Append the remaining part of the clipboard data
            string clip_line = clipboard.data.substr(pos);
            if (first_line) {
                current_line = before_cursor + clip_line + after_cursor;
            } else {
                lines[cursor_y + scroll_y] += clip_line;
            }

            // Move cursor
            for (size_t i = 0; i < clipboard.data.length(); i++) {
                if (clipboard.data[i] == '\n') {
                    cursor_x = 0;
                    cursor_y++;
                }
            }
            cursor_x += clipboard.data.length() - clipboard.data.find_last_of('\n') - 1;

            modified = true;
            draw_editor_area();
            draw_statusbar();
            terminal_setcursor(cursor_x, cursor_y);
        }
    }

    void handle_action(string action) {
        if (action == "new") {
            filename = "";
            content = "";
            cursor_x = 0;
            cursor_y = 1;
            parse_lines();
            modified = false;
            draw_statusbar();
            terminal_setcursor(cursor_x, cursor_y);
            draw_editor_area();
        } else if (action == "open") {
            char *cd = new char[cwd.length() + 1];
            strcpy(cd, cwd.c_str());
            char *new_filename = fileselector(cd, false);

            // Load file content
            if (new_filename != nullptr) {
                filename = string(new_filename);
                FILE *fp = fopen(filename.c_str(), "r");
                if (fp != nullptr) {
                    content.clear();
                    char buffer[1024];
                    size_t bytes_read;
                    while ((bytes_read = fread(buffer, 1, sizeof(buffer) - 1, fp)) > 0) {
                        buffer[bytes_read] = '\0';
                        content += string(buffer);
                    }
                    fclose(fp);
                    cursor_x = 0;
                    cursor_y = 1;
                }
            }

            delete[] cd;

            parse_lines();

            modified = false;
            draw_statusbar();
            cursor_x = 0;
            cursor_y = 1;
            terminal_setcursor(cursor_x, cursor_y);
        } else if (action == "save") {
            if (filename.empty()) {
                handle_action("saveas");
                return;
            }

            resolve_content();
            FILE *fp = fopen(filename.c_str(), "w");
            if (fp != nullptr) {
                fwrite(content.c_str(), 1, content.length(), fp);
                fclose(fp);
                modified = false;
            }
        } else if (action == "saveas") {
            char *cd = new char[cwd.length() + 1];
            strcpy(cd, cwd.c_str());
            char *new_filename = fileselector(cd, true);
            terminal_setcursor(cursor_x, cursor_y);

            // Save file content
            if (new_filename != nullptr) {
                filename = string(new_filename);
                resolve_content();
                FILE *fp = fopen(filename.c_str(), "w");
                if (fp != nullptr) {
                    fwrite(content.c_str(), 1, content.length(), fp);
                    fclose(fp);
                    modified = false;
                }
            }

            delete[] cd;
        } else if (action == "exit") {
            terminal_clearcursor();
            terminal_setcolor(0x07);
            terminal_clear();
            exit(0);
        } else if (action == "copy") {
            clipboard_copy();
        } else if (action == "cut") {
            clipboard_copy();
            delete_selection();
            modified = true;
            draw_editor_area();
            draw_statusbar();
            terminal_setcursor(cursor_x, cursor_y);
        } else if (action == "paste") {
            delete_selection();
            clipboard_paste();
        } else if (action == "about") {
            redraw();
            graphics.draw_rect(10, 5, 70, 18, 0xF0, 0x0F);
            graphics.put_text_centered(10, 7, 60, "EDIT.PRG");
            graphics.put_text_centered(10, 9, 60, "Version 1.0");
            graphics.put_text_centered(10, 11, 60, "by foliagecanine");
            string link = "http://github.com/foliagecanine/tritium-os";
            graphics.draw_rect((screen_width - link.length()) / 2, 13, (screen_width + link.length()) / 2, 14, 0xF1, 0xF1);
            graphics.put_text_centered(10, 13, 60, link.c_str());
            graphics.draw_rect((screen_width - 6) / 2, 15, (screen_width + 6) / 2, 16, 0x9F, 0x9F);
            graphics.put_text_centered(10, 15, 60, "OK");
            while (getchar() != '\n');
            while (getkey() & 0x80);
        }

        if (menu_selected != nullptr) {
            menu_selected->deselect();
            menu_selected = nullptr;
        }
    }

    void handle_right_arrow() {
        // If shift is pressed, start selection if not active
        if (shift_pressed) {
            if (!selection.active) {
                selection.start_x = cursor_x + scroll_x;
                selection.start_y = cursor_y + scroll_y;
                selection.end_x = selection.start_x;
                selection.end_y = selection.start_y;
                selection.active = true;
            } else {
                selection.end_x = cursor_x + scroll_x;
                selection.end_y = cursor_y + scroll_y;
            }
        } else {
            selection.active = false;
            draw_editor_area();
            draw_statusbar();
        }

        int current_line = cursor_y + scroll_y - 1;

        int old_scroll_x = scroll_x;
        int old_scroll_y = scroll_y;

        // Move cursor right
        cursor_x++;

        // If cursor is beyond the rightmost position, scroll right
        if (cursor_x >= screen_width) {
            cursor_x = screen_width - 1;
            scroll_x++;
        }

        desired_x = cursor_x + scroll_x;

        // If cursor is beyond the end of the line, move to the beginning of the next line
        if (cursor_x + scroll_x > lines[current_line].length()) {
            if (current_line + 1 < line_count) {
                cursor_x = 0;
                scroll_x = 0;
                cursor_y++;

                if (cursor_y >= screen_height - 1) {
                    cursor_y = screen_height - 2;
                    scroll_y++;
                }
            } else {
                cursor_x = lines[current_line].length() - scroll_x;
            }
        }

        desired_x = cursor_x + scroll_x;
        current_line = cursor_y + scroll_y - 1;

        // If the line is not empty and we are at the end, consider the cursor to be at line end for up/down movements
        if (desired_x == lines[current_line].length() && lines[current_line].length() > 0) {
            line_end = true;
        } else {
            line_end = false;
        }

        // Update selection end if shift is pressed
        if (shift_pressed) {
            selection.end_x = cursor_x + scroll_x;
            selection.end_y = cursor_y + scroll_y;
        }

        // Redraw if scrolled or selection changed
        if (old_scroll_x != scroll_x || old_scroll_y != scroll_y || shift_pressed) {
            draw_editor_area();
        }

        terminal_setcursor(cursor_x, cursor_y);
        draw_statusbar();
    }

    void handle_left_arrow() {
        // If shift is pressed, start selection if not active
        if (shift_pressed) {
            if (!selection.active) {
                selection.start_x = cursor_x + scroll_x;
                selection.start_y = cursor_y + scroll_y;
                selection.end_x = selection.start_x;
                selection.end_y = selection.start_y;
                selection.active = true;
            } else {
                selection.end_x = cursor_x + scroll_x;
                selection.end_y = cursor_y + scroll_y;
            }
        } else {
            selection.active = false;
            draw_editor_area();
            draw_statusbar();
        }

        int current_line = cursor_y + scroll_y - 1;
                    
        int old_scroll_x = scroll_x;
        int old_scroll_y = scroll_y;

        // If cursor is at leftmost position
        if (cursor_x == 0) {
            // Try scrolling left
            if (scroll_x > 0) {
                scroll_x--;
            // Else, move to end of previous line
            } else if (cursor_y > 1) {
                cursor_y--;
                current_line = cursor_y + scroll_y - 1;
                cursor_x = lines[current_line].length() - scroll_x;
                if (cursor_x >= screen_width) {
                    scroll_x = cursor_x - screen_width + 1;
                    cursor_x = screen_width - 1;
                } else {
                    scroll_x = 0;
                }
            }
        // Else, just move left
        } else {
            cursor_x--;
        }

        desired_x = cursor_x + scroll_x;
        current_line = cursor_y + scroll_y - 1;

        // If the line is not empty and we are at the end, consider the cursor to be at line end for up/down movements
        if (desired_x == lines[current_line].length() && lines[current_line].length() > 0) {
            line_end = true;
        } else {
            line_end = false;
        }

        // Update selection end if shift is pressed
        if (shift_pressed) {
            selection.end_x = cursor_x + scroll_x;
            selection.end_y = cursor_y + scroll_y;
        }

        // Redraw if scrolled or selection changed
        if (old_scroll_x != scroll_x || old_scroll_y != scroll_y || shift_pressed) {
            draw_editor_area();
        }

        terminal_setcursor(cursor_x, cursor_y);
        draw_statusbar();
    }

    void handle_down_arrow(bool page_down = false) {
        // If shift is pressed, start selection if not active
        if (shift_pressed) {
            if (!selection.active) {
                selection.start_x = cursor_x + scroll_x;
                selection.start_y = cursor_y + scroll_y;
                selection.end_x = selection.start_x;
                selection.end_y = selection.start_y;
                selection.active = true;
            } else {
                selection.end_x = cursor_x + scroll_x;
                selection.end_y = cursor_y + scroll_y;
            }
        } else {
            selection.active = false;
            draw_editor_area();
            draw_statusbar();
        }

        int current_line = cursor_y + scroll_y - 1;
        int old_scroll_x = scroll_x;
        int old_scroll_y = scroll_y;

        // Avoid going beyond last line
        if (current_line + 1 < line_count) {
            // Move cursor down
            cursor_y++;
            if (cursor_y >= screen_height - 1) {
                cursor_y = screen_height - 2;
                scroll_y++;
            }

            current_line = cursor_y + scroll_y - 1;

            // If we are at the end of the line, move to end of the new line
            if (line_end) {
                cursor_x = lines[current_line].length() - scroll_x;
                if (cursor_x >= screen_width) {
                    scroll_x = cursor_x - screen_width + 1;
                    cursor_x = screen_width - 1;
                } else {
                    scroll_x = 0;
                }
            // Else, maintain desired x position
            } else {
                if (desired_x < lines[current_line].length()) {
                    cursor_x = desired_x - scroll_x;
                    if (cursor_x >= screen_width) {
                        scroll_x = desired_x - screen_width + 1;
                        cursor_x = screen_width - 1;
                    }
                } else {
                    cursor_x = lines[current_line].length() - scroll_x;
                    if (cursor_x >= screen_width) {
                        scroll_x = cursor_x - screen_width + 1;
                        cursor_x = screen_width - 1;
                    } else {
                        scroll_x = 0;
                    }
                }
            } 
        }

        // Update selection end if shift is pressed
        if (shift_pressed) {
            selection.end_x = cursor_x + scroll_x;
            selection.end_y = cursor_y + scroll_y;
        }

        // Redraw if scrolled or selection changed
        if (!page_down) {
            if (old_scroll_x != scroll_x || old_scroll_y != scroll_y || shift_pressed) {
                draw_editor_area();
            }

            terminal_setcursor(cursor_x, cursor_y);
            draw_statusbar();
        }
    }

    void handle_up_arrow(bool page_up = false) {
        // If shift is pressed, start selection if not active
        if (shift_pressed) {
            if (!selection.active) {
                selection.start_x = cursor_x + scroll_x;
                selection.start_y = cursor_y + scroll_y;
                selection.end_x = selection.start_x;
                selection.end_y = selection.start_y;
                selection.active = true;
            } else {
                selection.end_x = cursor_x + scroll_x;
                selection.end_y = cursor_y + scroll_y;
            }
        } else {
            selection.active = false;
            draw_editor_area();
            draw_statusbar();
        }

        int current_line = cursor_y + scroll_y - 1;

        int old_scroll_x = scroll_x;
        int old_scroll_y = scroll_y;

        // Avoid going above first line
        if (current_line > 0) {
            // Move cursor up
            cursor_y--;
            if (cursor_y < 1) {
                cursor_y = 1;
                scroll_y--;
            }

            current_line = cursor_y + scroll_y - 1;

            // If we are at the end of the line, move to end of the new line
            if (line_end) {
                cursor_x = lines[current_line].length() - scroll_x;
                if (cursor_x >= screen_width) {
                    scroll_x = cursor_x - screen_width + 1;
                    cursor_x = screen_width - 1;
                } else {
                    scroll_x = 0;
                }
            // Else, maintain desired x position
            } else {
                if (desired_x < lines[current_line].length()) {
                    cursor_x = desired_x - scroll_x;
                    if (cursor_x >= screen_width) {
                        scroll_x = desired_x - screen_width + 1;
                        cursor_x = screen_width - 1;
                    }
                } else {
                    cursor_x = lines[current_line].length() - scroll_x;
                    if (cursor_x >= screen_width) {
                        scroll_x = cursor_x - screen_width + 1;
                        cursor_x = screen_width - 1;
                    } else {
                        scroll_x = 0;
                    }
                }
            } 
        }

        // Update selection end if shift is pressed
        if (shift_pressed) {
            selection.end_x = cursor_x + scroll_x;
            selection.end_y = cursor_y + scroll_y;
        }

        // Redraw if scrolled or selection changed
        if (!page_up) {
            if (old_scroll_x != scroll_x || old_scroll_y != scroll_y || shift_pressed) {
                draw_editor_area();
            }
            
            terminal_setcursor(cursor_x, cursor_y);
            draw_statusbar();
        }
    }

    void handle_end_key() {
        if (shift_pressed) {
            if (!selection.active) {
                selection.start_x = cursor_x + scroll_x;
                selection.start_y = cursor_y + scroll_y;
                selection.end_x = selection.start_x;
                selection.end_y = selection.start_y;
                selection.active = true;
            }
        } else {
            selection.active = false;
            draw_editor_area();
            draw_statusbar();
        }

        int current_line = cursor_y + scroll_y - 1;

        int old_scroll_x = scroll_x;
        int old_scroll_y = scroll_y;

        cursor_x = lines[current_line].length() - scroll_x;
        if (cursor_x >= screen_width) {
            scroll_x = cursor_x - screen_width + 1;
            cursor_x = screen_width - 1;
        } else {
            scroll_x = 0;
        }

        desired_x = cursor_x + scroll_x;

        if (shift_pressed) {
            selection.end_x = cursor_x + scroll_x;
            selection.end_y = cursor_y + scroll_y;
        }

        // Redraw if scrolled or selection changed
        if (old_scroll_x != scroll_x || old_scroll_y != scroll_y || shift_pressed) {
            draw_editor_area();
        }

        terminal_setcursor(cursor_x, cursor_y);
        draw_statusbar();
    }

    void handle_home_key() {
        if (shift_pressed) {
            if (!selection.active) {
                selection.start_x = cursor_x + scroll_x;
                selection.start_y = cursor_y + scroll_y;
                selection.end_x = selection.start_x;
                selection.end_y = selection.start_y;
                selection.active = true;
            }
        } else {
            selection.active = false;
            draw_editor_area();
            draw_statusbar();
        }

        int old_scroll_x = scroll_x;
        int old_scroll_y = scroll_y;

        cursor_x = 0;
        scroll_x = 0;

        desired_x = cursor_x + scroll_x;

        if (shift_pressed) {
            selection.end_x = cursor_x + scroll_x;
            selection.end_y = cursor_y + scroll_y;
        }

        // Redraw if scrolled or selection changed
        if (old_scroll_x != scroll_x || old_scroll_y != scroll_y || shift_pressed) {
            draw_editor_area();
        }

        terminal_setcursor(cursor_x, cursor_y);
        draw_statusbar();
    }

    void handle_pagedown_key() {
        for (unsigned int i = 0; i < screen_height - 2; i++) {
            handle_down_arrow(true);
        }

        draw_editor_area();
        terminal_setcursor(cursor_x, cursor_y);
        draw_statusbar();
    }

    void handle_pageup_key() {
        for (unsigned int i = 0; i < screen_height - 2; i++) {
            handle_up_arrow(true);
        }

        draw_editor_area();
        terminal_setcursor(cursor_x, cursor_y);
        draw_statusbar();
    }

    void delete_selection() {
        // Handle deletion of selected text
        if (!selection.active) {
            return;
        }

        unsigned int sel_start_y = selection.start_y - 1;
        unsigned int sel_end_y = selection.end_y - 1;
        unsigned int sel_start_x = selection.start_x;
        unsigned int sel_end_x = selection.end_x;

        if (sel_start_y > sel_end_y || (sel_start_y == sel_end_y && sel_start_x > sel_end_x)) {
            swap(sel_start_y, sel_end_y);
            swap(sel_start_x, sel_end_x);
        }

        string *new_lines = new string[line_count - (sel_end_y - sel_start_y)];
        unsigned int new_line_index = 0;

        for (unsigned int i = 0; i < line_count; i++) {
            if (i < sel_start_y || i > sel_end_y) {
                new_lines[new_line_index] = lines[i];
                new_line_index++;
            } else if (i == sel_start_y) {
                string before_selection = lines[i].substr(0, sel_start_x);
                string after_selection;
                if (sel_start_y == sel_end_y) {
                    after_selection = lines[i].substr(sel_end_x, lines[i].length() - sel_end_x);
                } else {
                    after_selection = lines[sel_end_y].substr(sel_end_x, lines[sel_end_y].length() - sel_end_x);
                }
                new_lines[new_line_index] = before_selection + after_selection;
                new_line_index++;
            }
        }

        delete[] lines;
        lines = new_lines;
        line_count -= (sel_end_y - sel_start_y);

        cursor_y = sel_start_y + 1;
        cursor_x = sel_start_x;
        scroll_x = 0;
        scroll_y = 0;
        desired_x = cursor_x + scroll_x;

        if (cursor_y >= screen_height - 1) {
            cursor_y = screen_height - 2;
            scroll_y = cursor_y + scroll_y - (screen_height - 2);
        }

        if (cursor_x >= screen_width) {
            scroll_x = cursor_x - screen_width + 1;
            cursor_x = screen_width - 1;
        }

        selection.active = false;
        draw_editor_area();
        draw_statusbar();
    }

    void handle_printable_char(char curr_char) {
        if (selection.active) {
            delete_selection();
        }

        int current_line = cursor_y + scroll_y - 1;

        // Insert character at cursor position
        lines[current_line].insert(cursor_x + scroll_x, string(1, curr_char));

        // Move cursor right, scrolling if needed
        cursor_x++;
        if (cursor_x >= screen_width) {
            cursor_x = screen_width - 1;
            scroll_x++;
        }

        desired_x = cursor_x + scroll_x;
        current_line = cursor_y + scroll_y - 1;

        // Redraw editor area
        draw_editor_area();
        terminal_setcursor(cursor_x, cursor_y);
        modified = true;
        draw_statusbar();
    }

    void handle_enter_key() {
        if (selection.active) {
            delete_selection();
        }

        string *new_lines = new string[line_count + 1];

        // Copy lines, except we split at cursor position
        for (unsigned int i = 0; i < line_count + 1; i++) {
            if (i < cursor_y + scroll_y) {
                new_lines[i] = lines[i];
            } else if (i == cursor_y + scroll_y) {
                unsigned int current_line = cursor_y + scroll_y - 1;
                new_lines[i] = lines[current_line].substr(cursor_x + scroll_x, lines[current_line].length() - (cursor_x + scroll_x));
                new_lines[i - 1] = lines[current_line].substr(0, cursor_x + scroll_x);
            } else {
                new_lines[i] = lines[i - 1];
            }
        }

        delete[] lines;
        lines = new_lines;
        line_count++;

        // Increment cursor y position, and if needed, scroll down
        cursor_y++;
        cursor_x = 0;
        scroll_x = 0;
        if (cursor_y >= screen_height - 1) {
            cursor_y = screen_height - 2;
            scroll_y++;
        }

        desired_x = cursor_x + scroll_x;
        
        line_end = false;
        draw_editor_area();
        terminal_setcursor(cursor_x, cursor_y);
        modified = true;
        draw_statusbar();
    }

    void handle_backspace_key() {
        if (selection.active) {
            delete_selection();

            // Redraw editor area
            draw_editor_area();
            terminal_setcursor(cursor_x, cursor_y);
            modified = true;
            draw_statusbar();

            return;
        }

        int current_line = cursor_y + scroll_y - 1;

        // If cursor is not at the start of the line
        if (cursor_x + scroll_x > 0) {
            // Remove character before cursor
            lines[current_line].erase(cursor_x + scroll_x - 1, 1);

            // Move cursor left, scrolling if needed
            if (cursor_x == 0) {
                if (scroll_x > 0) {
                    scroll_x--;
                } else if (cursor_y > 1) {
                    cursor_y--;
                    current_line = cursor_y + scroll_y - 1;
                    cursor_x = lines[current_line].length() - scroll_x;
                    if (cursor_x >= screen_width) {
                        scroll_x = cursor_x - screen_width + 1;
                        cursor_x = screen_width - 1;
                    } else {
                        scroll_x = 0;
                    }
                }
            } else {
                cursor_x--;
            }

            desired_x = cursor_x + scroll_x;
            current_line = cursor_y + scroll_y - 1;

        // If cursor is at the beginning of a line, merge with previous line
        } else if (current_line > 0) {
            lines[current_line - 1] += lines[current_line];
            
            for (unsigned int i = current_line; i < line_count - 1; i++) {
                lines[i] = lines[i + 1];
            }
            line_count--;

            cursor_y--;
            if (cursor_y < 1) {
                cursor_y = 1;
                scroll_y--;
            }

            // Scroll to end of previous line
            current_line = cursor_y + scroll_y - 1;
            cursor_x = lines[current_line].length() - scroll_x;
            if (cursor_x >= screen_width) {
                scroll_x = cursor_x - screen_width + 1;
                cursor_x = screen_width - 1;
            } else {
                scroll_x = 0;
            }
        }

        // If the line is not empty and we are at the end, consider the cursor to be at line end for up/down movements
        if (desired_x == lines[current_line].length() && lines[current_line].length() > 0) {
            line_end = true;
        } else {
            line_end = false;
        }

        // Redraw editor area
        draw_editor_area();
        terminal_setcursor(cursor_x, cursor_y);
        modified = true;
        draw_statusbar();
    }

    void handle_delete_key() {
        if (selection.active) {
            delete_selection();

            // Redraw editor area
            draw_editor_area();
            terminal_setcursor(cursor_x, cursor_y);
            modified = true;
            draw_statusbar();

            return;
        }

        int current_line = cursor_y + scroll_y - 1;

        // If cursor is not at the end of the line
        if (cursor_x + scroll_x < lines[current_line].length()) {
            // Remove character at cursor
            lines[current_line].erase(cursor_x + scroll_x, 1);

            // Redraw editor area
            draw_editor_area();
            terminal_setcursor(cursor_x, cursor_y);
            modified = true;
            draw_statusbar();
        // If cursor is at the end of the line, merge with next line
        } else if (current_line + 1 < line_count) {
            lines[current_line] += lines[current_line + 1];
            
            for (unsigned int i = current_line + 1; i < line_count - 1; i++) {
                lines[i] = lines[i + 1];
            }
            line_count--;

            // Redraw editor area
            draw_editor_area();
            terminal_setcursor(cursor_x, cursor_y);
            modified = true;
            draw_statusbar();
        }
    }

    void run() {
        // terminal_clear();
        terminal_enablecursor(0, 15);
	    terminal_setcursor(cursor_x, cursor_y);
        redraw();

        unsigned int curr_key = getkey();
        char curr_char = getchar();

        while (curr_key != 0x01) {
            if (curr_key == KBDIN_KEY_LALT) {
                alt_pressed = true;
                terminal_clearcursor();
                draw_toolbar(false);
            } else if (curr_key == KBDIN_KEY_LALT + KBDIN_KEY_RELEASED) {
                alt_pressed = false;

                if (menu_selected != nullptr) {
                    menu_selected->deselect();
                    menu_selected = nullptr;
                }

                terminal_enablecursor(0, 15);
                redraw();
            } else if (curr_key == KBDIN_KEY_LSHIFT || curr_key == KBDIN_KEY_RSHIFT) {
                shift_pressed = true;
            } else if (curr_key == KBDIN_KEY_LSHIFT + KBDIN_KEY_RELEASED || curr_key == KBDIN_KEY_RSHIFT + KBDIN_KEY_RELEASED) {
                shift_pressed = false;
            } else if (curr_key == KBDIN_KEY_LCONTROL) {
                ctrl_pressed = true;
            } else if (curr_key == KBDIN_KEY_LCONTROL + KBDIN_KEY_RELEASED) {
                ctrl_pressed = false;
            }

            if (alt_pressed && !shift_pressed && !ctrl_pressed) {
                // Handle menu navigation
                if (menu_selected == nullptr && curr_char != '\0') {
                    ToolbarMenu *menu = first_toolbar_menu;
                    while (menu != nullptr) {
                        if (tolower(curr_char) == menu->get_activation_key()) {
                            menu_selected = menu;
                            menu_selected->select();
                            draw_toolbar(false);
                            break;
                        }
                        menu = menu->get_next();
                    }
                // Handle menu item selection
                } else if (menu_selected != nullptr) {
                    ToolbarMenuItem *item = menu_selected->get_first_item();
                    while (item != nullptr) {
                        if (tolower(curr_char) == item->get_activation_key()) {
                            handle_action(item->get_action());
                            alt_pressed = false;
                            terminal_enablecursor(0, 15);
                            redraw();
                            break;
                        }
                        item = item->get_next();
                    }
                }
            } else if (ctrl_pressed && !alt_pressed && !shift_pressed) {
                switch (curr_char) {
                    case 'a':
                        selection.active = true;
                        selection.start_x = 0;
                        selection.start_y = 1;
                        selection.end_x = lines[line_count - 1].length();
                        selection.end_y = line_count;
                        cursor_x = lines[line_count - 1].length() < screen_width ? lines[line_count - 1].length() : screen_width - 1;
                        cursor_y = line_count < screen_height - 1 ? line_count : screen_height - 2;
                        scroll_x = lines[line_count - 1].length() > screen_width ? lines[line_count - 1].length() - (screen_width - 1) : 0;
                        scroll_y = line_count > screen_height - 2 ? line_count - (screen_height - 2) : 0;
                        desired_x = cursor_x + scroll_x;
                        line_end = true;
                        terminal_setcursor(cursor_x, cursor_y);
                        draw_editor_area();
                        draw_statusbar();
                    case 'c':
                        handle_action("copy");
                        break;
                    case 'x':
                        handle_action("cut");
                        break;
                    case 'v':
                        handle_action("paste");
                        break;
                    case 's':
                        handle_action("save");
                        break;
                    default:
                        break;
                }
            } else {
                switch (curr_key) {
                    case KBDIN_KEY_RIGHTARROW:
                        handle_right_arrow();
                        break;
                    case KBDIN_KEY_LEFTARROW:
                        handle_left_arrow();
                        break;
                    case KBDIN_KEY_UPARROW:
                        handle_up_arrow();
                        break;
                    case KBDIN_KEY_DOWNARROW:
                        handle_down_arrow();
                        break;
                    case KBDIN_KEY_ENTER:
                        handle_enter_key();
                        break;
                    case KBDIN_KEY_BACKSPACE:
                        handle_backspace_key();
                        break;
                    case KBDIN_KEY_DELETE:
                        handle_delete_key();
                        break;
                    case KBDIN_KEY_HOME:
                        handle_home_key();
                        break;
                    case KBDIN_KEY_END:
                        handle_end_key();
                        break;
                    case KBDIN_KEY_PAGEDOWN:
                        handle_pagedown_key();
                        break;
                    case KBDIN_KEY_PAGEUP:
                        handle_pageup_key();
                        break;
                    default:
                        if (curr_char >= ' ' && curr_char <= '~') {
                            handle_printable_char(curr_char);
                        }
                        break;
                }
            }
            
            curr_key = getkey();
            curr_char = getchar();
        }
    }
};

int main(int argc, char** argv) {
    string cd = string(getenv("CD"));

    string filename = "";
    if (argc > 1) {
        filename = string(argv[1]);
    }
    
    Editor editor(cd, filename);
    editor.run();
    terminal_clearcursor();
    terminal_setcolor(0x07);
    // terminal_clear();
    return 0;
}
