#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <utility>

//#include "problem_c.h"
#include "capacitance.h"

using namespace std;

void ReadConfig()
{
    ifstream file(config_file);

    string str;
    while (file >> str) {
        if (str == "design:")
            file >>  circuit_file;
        else if (str == "output:")
            file >> output_file;
        else if (str == "rule_file:")
            file >> rule_file;
        else if (str == "process_file:")
            file >> process_file;
        else if (str == "critical_nets:") {
            getline(file, str);
            char *ch = strtok((char *)str.c_str(), " ");
            while (ch != NULL) {
                critical_nets.insert(atoi(ch));
                ch = strtok(NULL, " ");
            }
        }
        else if (str == "power_nets:")
            file >> power_net;
        else if (str == "ground_net:")
            file >> ground_net;
    }

    file.close();
}

void ReadCircuit()
{
    ifstream file(path + circuit_file);

    string str;
    getline(file, str);
    size_t pos = str.find(" ");
    cb.bl_x = atoi((char *)str.substr(0, pos).c_str());
    str.erase(0, pos + 1);
    pos = str.find(" ");
    cb.bl_y = atoi((char *)str.substr(0, pos).c_str());
    str.erase(0, pos + 1);
    pos = str.find(" ");
    cb.tr_x = atoi((char *)str.substr(0, pos).c_str());
    str.erase(0, pos + 1);
    pos = str.find(";");
    cb.tr_y = atoi((char *)str.substr(0, pos).c_str());
    str.erase(0, pos + 1);
    cb.width_x = cb.tr_x - cb.bl_x;
    cb.width_y = cb.tr_y - cb.bl_y;

    Layout l = {.id = 0, .bl_x = -1, .bl_y = -1, .tr_x = -1, .tr_y = -1, 
                .net_id = -1, .layer = -1, .type = -1, .isCritical = false};
    layouts.emplace_back(l); // dummy layout to align id and index;
    while (file >> l.id >> l.bl_x >> l.bl_y >> l.tr_x >> l.tr_y >> l.net_id >> l.layer >> str) {
        l.bl_x -= cb.bl_x;
        l.bl_y -= cb.bl_y;
        l.tr_x -= cb.bl_x;
        l.tr_y -= cb.bl_y;
        if (str == "Drv_Pin")
            l.type = 0;
        else if (str == "Normal" || str == "normal")
            l.type = 1;
        else if (str == "Load_Pin")
            l.type = 2;
        else if (str == "Fill")
            l.type = 3;
        else {
            cout << "Invalid Polygon Type: " << str << '\n';
            exit(1);
        }
        l.isCritical = critical_nets.find(l.net_id) != critical_nets.end();

        layouts.emplace_back(l);

    //     // get or create the head node of that layer
    //     if (metals.size() < l.layer) {
    //         Metal *head = new Metal(l.id);
    //         metals.emplace_back(head);
    //         continue;
    //     }
    //     Metal *metal = metals[l.layer - 1];

    //     Layout temp = layouts[metal->id];
    //     // sort order: x -> y
    //     // if x < head.x, replace head node
    //     if (l.bl_x < temp.bl_x) {
    //         Metal *new_head = new Metal(l.id);
    //         new_head->next_x = metal;
    //         metals[l.layer - 1] = new_head;
    //         continue;
    //     }
    //     // traverse in x direction
    //     Metal *prev = metal;
    //     while (metal != NULL) {
    //         temp = layouts[metal->id];
    //         if (l.bl_x <= temp.bl_x)
    //             break;
    //         prev = metal;
    //         metal = metal->next_x;
    //     }
    //     // traverse to last node or smaller means there is no exist node with that x
    //     if (metal == NULL || l.bl_x < temp.bl_x) {
    //         // prev -> new_metal -> metal
    //         Metal *new_metal = new Metal(l.id);
    //         prev->next_x = new_metal;
    //         new_metal->next_x = metal;
    //         continue;
    //     }
    //     // same x, traverse in y direction
    //     // if y < metal.y, replace y dircetion head node
    //     if (l.bl_y < temp.bl_y) {
    //         Metal *new_metal = new Metal(l.id);
    //         if (metal == metals[l.layer - 1]) {
    //             new_metal->next_x = metal->next_x;
    //             new_metal->next_y = metal;
    //             metal->next_x = NULL;
    //             metals[l.layer - 1] = new_metal;
    //         }
    //         else {
    //             // x direction link
    //             new_metal->next_x = metal->next_x;
    //             prev->next_x = new_metal;
    //             // y direction link
    //             metal->next_x = NULL;
    //             new_metal->next_y = metal;
    //         }
    //         continue;
    //     }
    //     prev = metal;
    //     // traverse in y direction
    //     while (metal != NULL) {
    //         temp = layouts[metal->id];
    //         if (l.bl_y < temp.bl_y) // impossible same y
    //             break;
    //         prev = metal;
    //         metal = metal->next_y;
    //     }
    //     // prev -> new_metal -> metal
    //     Metal *new_metal = new Metal(l.id);
    //     prev->next_y = new_metal;
    //     new_metal->next_y = metal;
    }

    file.close();
    
    total_metals = layouts.size() - 1;
    total_layers = layouts[total_metals - 1].layer;
}

void ReadProcess()
{
    ifstream file(path + process_file);

    string str;
    getline(file, str); // comment line
    getline(file, str);
    window_size = atoi((char *)str.substr(str.find(" ") + 1, str.length()).c_str());
    getline(file, str); // comment line
    getline(file, str); // comment line

    getline(file, str); // col indices line
    for (int row = 0; row < total_layers + 1; row++) {
        getline(file, str);
        for (int col = 0; col < total_layers; col++) {
            size_t start = str.find("(");
            size_t mid = str.find(",");
            size_t end = str.find(")");
            area_tables.emplace_back(str.substr(start + 1, mid - start - 1));
            fringe_tables.emplace_back(str.substr(mid + 2, end - mid - 2));
            str[start] = ';';
            str[mid] = ';';
            str[end] = ';';
        }
    }

    // useless lines
    getline(file, str);
    getline(file, str);
    getline(file, str);
    getline(file, str);
    getline(file, str);

    int total_area_tables = 0;
    for (int i = 1; i <= total_layers; i++)
        total_area_tables += i;
    int total_lateral_tables = total_layers;
    int total_fringe_tables = fringe_tables.size() - total_layers * 2;

    string table_name;
    for (int i = 0; i < total_area_tables; i++) {
        AreaTable atbl;
        file >> str >> table_name;
        getline(file, str); // previous line
        getline(file, str); // comment line

        getline(file, str);
        char *ch = strtok((char *)str.c_str(), " ");
        while (ch != NULL) {
            atbl.s.emplace_back(atof(ch));
            ch = strtok(NULL, " ");
        }

        getline(file, str);
        size_t start = str.find("(");
        size_t mid = str.find(",");
        size_t end = str.find(")");
        while (start != string::npos) {
            atbl.a.emplace_back(atof((char *)str.substr(start + 1, mid - start - 1).c_str()));
            atbl.b.emplace_back(atof((char *)str.substr(mid + 2, end - mid - 2).c_str()));
            str[start] = ';';
            str[mid] = ';';
            str[end] = ';';
            start = str.find("(");
            mid = str.find(",");
            end = str.find(")");
        }

        getline(file, str); // empty line

        area_table_map[table_name] = atbl;
    }

    // empty lines
    getline(file, str);
    getline(file, str);
    getline(file, str);

    for (int i = 0; i < total_lateral_tables; i++) {
        FringeTable ftbl;
        file >> str >> table_name;
        getline(file, str); // previous line
        getline(file, str); // comment line

        getline(file, str);
        char *ch = strtok((char *)str.c_str(), " ");
        while (ch != NULL) {
            ftbl.d.emplace_back(atof(ch));
            ch = strtok(NULL, " ");
        }

        getline(file, str);
        size_t start = str.find("(");
        size_t mid = str.find(",");
        size_t end = str.find(")");
        if (start != string::npos) {
            ftbl.a.emplace_back(stod(str.substr(start + 1, mid - start - 1)));
            ftbl.b.emplace_back(stod(str.substr(mid + 2, end - mid - 2)));
            str[start] = ';';
            str[mid] = ';';
            str[end] = ';';
        }

        getline(file, str); //empty line

        fringe_table_map[table_name] = ftbl;
    }

    // empty lines
    getline(file, str);
    getline(file, str);
    getline(file, str);
    getline(file, str);

    for (int i = 0; i < total_fringe_tables; i++) {
        FringeTable ftbl;
        file >> str >> table_name;
        getline(file, str); // previous line
        getline(file, str); // comment line

        getline(file, str);
        char *ch = strtok((char *)str.c_str(), " ");
        while (ch != NULL) {
            ftbl.d.emplace_back(atof(ch));
            ch = strtok(NULL, " ");
        }

        getline(file, str);
        size_t start = str.find("(");
        size_t mid = str.find(",");
        size_t end = str.find(")");
        if (start != string::npos) {
            ftbl.a.emplace_back(stod(str.substr(start + 1, mid - start - 1)));
            ftbl.b.emplace_back(stod(str.substr(mid + 2, end - mid - 2)));
            str[start] = ';';
            str[mid] = ';';
            str[end] = ';';
        }

        getline(file, str); //empty line

        fringe_table_map[table_name] = ftbl;
    }

    file.close();
}

void ReadRule()
{
    ifstream file(path + rule_file);

    Rule r;
    string str;
    long long w_area = (long long)window_size * window_size;
    long long qw_area = (long long)stride * stride;
    while (file >> r.layer >> str >> r.min_width >> r.min_space >> r.max_fill_width >> 
           r.min_density >> r.max_density)
        rules.emplace_back(r);

    file.close();
}

void AnalyzeDensity()
{
    stride = window_size / 2;
    long long w_area = (long long)window_size * window_size; 
    long long qw_area = (long long)stride * stride;
    // # windows
    window_x = cb.width_x / window_size * 2 - 1;
    window_y = cb.width_y / window_size * 2 - 1;
    // # quarter-windows
    qwindow_x = cb.width_x / stride;
    qwindow_y = cb.width_y / stride;

    int current_metal = 1;
    for (int layer = 1; layer <= total_layers; layer++) {
        // initialize quarter-window
        vector<QuarterWindow> qwindows(qwindow_x * qwindow_y);
        for (int x = 0; x < qwindow_x; x++) {
            int start = x * stride;
            int end = (x + 1) * stride;
            for (int y = 0; y < qwindow_y; y++) {
                int index = x * qwindow_y + y;
                qwindows[index].index = index;
                qwindows[index].area = 0;
                qwindows[index].bl_x = start;
                qwindows[index].tr_x = end;
                qwindows[index].bl_y = y * stride;
                qwindows[index].tr_y = (y + 1) * stride;
                qwindows[index].violate_count = 0;
                qwindows[index].hasCritical = 0;
            }
        }

        // layout corrsponds to metal index
        Layout temp = layouts[current_metal];
        // index of metal in which quarter-window
        int x_from, x_to, y_from, y_to;
        // coordinate of partial metal in quarter window
        int x_start, x_end, y_start, y_end;
        long long area = 0;
        while (layer == temp.layer) {
		    x_from = temp.bl_x / stride;
		    x_to = (temp.tr_x - 1) / stride;
		    y_from = temp.bl_y / stride;
		    y_to = (temp.tr_y - 1) / stride;
		    for (int x = x_from; x <= x_to; x++) {
			    x_start = x == x_from ? temp.bl_x : x * stride;
			    x_end = x == x_to ? temp.tr_x : (x + 1) * stride;
			    for (int y = y_from; y <= y_to; y++) {
				    y_start = y == y_from ? temp.bl_y : y * stride;
				    y_end = y == y_to ? temp.tr_y : (y + 1) * stride;
				    area = (x_end - x_start) * (y_end - y_start);
				    qwindows[x * qwindow_y + y].area += area;
                    qwindows[x * qwindow_y + y].contribute_metals.emplace_back(temp.id);
                    if (temp.isCritical)
                        qwindows[x * qwindow_y + y].hasCritical++;
			    }
		    }
            if (++current_metal == total_metals + 1)
                break;
            temp = layouts[current_metal];
        }

        vector<Window> ws(window_x * window_y);
        long long min_area = w_area * rules[layer - 1].min_density;
        double min_density = rules[layer - 1].min_density;
        for (int x = 0; x < window_x; x++) {
            for (int y = 0; y < window_y; y++) {
                // window index
                int index = x * window_y + y;
                // quarter window index
                int q_index = x * qwindow_y + y;

                ws[index].index = index;
                ws[index].area = qwindows[q_index].area + qwindows[q_index + qwindow_y].area +
                                 qwindows[q_index + 1].area + qwindows[q_index + qwindow_y + 1].area;
                ws[index].area_insufficient = 0;
                ws[index].density = (double)ws[index].area / w_area;
                // remember which quarter window is included
                ws[index].included_qwindow.emplace_back(q_index);
                ws[index].included_qwindow.emplace_back(q_index + qwindow_y);
                ws[index].included_qwindow.emplace_back(q_index + 1);
                ws[index].included_qwindow.emplace_back(q_index + qwindow_y + 1);

                // remember which window will be affected
                qwindows[q_index].affected_window.emplace_back(index);
                qwindows[q_index + qwindow_y].affected_window.emplace_back(index);
                qwindows[q_index + 1].affected_window.emplace_back(index);
                qwindows[q_index + qwindow_y + 1].affected_window.emplace_back(index);

                if (ws[index].density < min_density) {
                    ws[index].area_insufficient = min_area - ws[index].area;
                    qwindows[q_index].violate_count++;
                    qwindows[q_index + qwindow_y].violate_count++;
                    qwindows[q_index + 1].violate_count++;
                    qwindows[q_index + qwindow_y + 1].violate_count++; 
                }
            }
        }

        quarter_windows.emplace_back(qwindows);
        windows.emplace_back(ws);
    }
}

void FindSpace(vector<Rect> &rts, const QuarterWindow &qw, const int min_width, const int min_space)
{
    int actual_min_width = min_width + 2 * min_space;

    Rect qw_rect = {.bl_x = qw.bl_x, .bl_y = qw.bl_y, .tr_x = qw.tr_x, .tr_y = qw.tr_y, .width_x = stride, .width_y = stride};
    rts.emplace_back(qw_rect);

    for (int metal_id : qw.contribute_metals) {
        Layout &temp = layouts[metal_id];
        vector<Rect> temp_rts;
        for (Rect rt : rts) {
            // if not overlap then add
            if (rt.bl_x >= temp.tr_x || rt.tr_x <= temp.bl_x || rt.bl_y >= temp.tr_y || rt.tr_y <= temp.tr_y) {
                temp_rts.emplace_back(rt);
                continue;
            }

            // intersect types
            int bit0 = rt.bl_x < temp.bl_x ? 0 : 1;
            int bit1 = rt.bl_y < temp.bl_y ? 0 : 2;
            int bit2 = rt.tr_x > temp.tr_x ? 0 : 4;
            int bit3 = rt.tr_y > temp.tr_y ? 0 : 8;
            int condition = bit0 + bit1 + bit2 + bit3;
            
            Rect rt_new;
            // middle (rt.bl_x < temp.bl_x && rt.bl_y < temp.bl_y && rt.tr_x > temp.tr_x && rt.tr_y > temp.tr_y)
            if (condition == 0) {
                // four new rects
                if (temp.tr_x - temp.bl_x > temp.tr_y - temp.bl_y) {
                    // top
                    rt_new.bl_x = rt.bl_x;
                    rt_new.bl_y = temp.tr_y;
                    rt_new.tr_x = rt.tr_x;
                    rt_new.tr_y = rt.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    // bottom
                    rt_new.bl_y = rt.bl_y;
                    rt_new.tr_y = temp.bl_y;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    // left
                    rt_new.bl_x = rt.bl_x;
                    rt_new.bl_y = temp.bl_y;
                    rt_new.tr_x = temp.bl_x;
                    rt_new.tr_y = temp.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    // right
                    rt_new.bl_x = temp.tr_x;
                    rt_new.tr_x = rt.tr_x;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                }
                else {
                    // left
                    rt_new.bl_x = rt.bl_x;
                    rt_new.bl_y = rt.bl_y;
                    rt_new.tr_x = temp.bl_x;
                    rt_new.tr_y = rt.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    // right
                    rt_new.bl_x = temp.tr_x;
                    rt_new.tr_x = rt.tr_x;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    // top
                    rt_new.bl_x = temp.bl_x;
                    rt_new.bl_y = temp.tr_y;
                    rt_new.tr_x = temp.tr_x;
                    rt_new.tr_y = rt.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    // bottom
                    rt_new.bl_y = rt.bl_y;
                    rt_new.tr_y = temp.bl_y;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                }
            }
            // left middle (rt.bl_x >= temp.bl_x && rt.bl_y < temp.bl_y && rt.tr_x > temp.tr_x && rt.tr_y > temp.tr_y)
            else if (condition == 1) {
                // three new rects
                if (temp.tr_x -  rt.bl_x > temp.tr_y - temp.bl_y) {
                    // top
                    rt_new.bl_x = rt.bl_x;
                    rt_new.bl_y = temp.tr_y;
                    rt_new.tr_x = rt.tr_x;
                    rt_new.tr_y = rt.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    // bottom
                    rt_new.bl_y = rt.bl_y;
                    rt_new.tr_y = temp.bl_y;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    // right
                    rt_new.bl_x = temp.tr_x;
                    rt_new.bl_y = temp.bl_y;
                    rt_new.tr_x = rt.tr_x;
                    rt_new.tr_y = temp.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                }
                else {
                    // top
                    rt_new.bl_x = rt.bl_x;
                    rt_new.bl_y = temp.tr_y;
                    rt_new.tr_x = temp.tr_x;
                    rt_new.tr_y = rt.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    // bottom
                    rt_new.bl_y = rt.bl_y;
                    rt_new.tr_y = temp.bl_y;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    // right
                    rt_new.bl_x = temp.tr_x;
                    rt_new.bl_y = rt.bl_y;
                    rt_new.tr_x = rt.tr_x;
                    rt_new.tr_y = rt.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                }
            }
            // bottom middle (rt.bl_x < temp.bl_x && rt.bl_y >= temp.bl_y && rt.tr_x > temp.tr_x && rt.tr_y > temp.tr_y)
            else if (condition == 2) {
                // three new rects
                if (temp.tr_x - temp.bl_x > temp.tr_y - rt.bl_y) {
                    // top
                    rt_new.bl_x = rt.bl_x;
                    rt_new.bl_y = temp.tr_y;
                    rt_new.tr_x = rt.tr_x;
                    rt_new.tr_y = rt.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    // left
                    rt_new.bl_x = rt.bl_x;
                    rt_new.bl_y = rt.bl_y;
                    rt_new.tr_x = temp.bl_x;
                    rt_new.tr_y = temp.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    // right
                    rt_new.bl_x = temp.tr_x;
                    rt_new.tr_x = rt.tr_x;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                }
                else {
                    // top
                    rt_new.bl_x = temp.bl_x;
                    rt_new.bl_y = temp.tr_y;
                    rt_new.tr_x = temp.tr_x;
                    rt_new.tr_y = rt.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    // left
                    rt_new.bl_x = rt.bl_x;
                    rt_new.bl_y = rt.bl_y;
                    rt_new.tr_x = temp.bl_x;
                    rt_new.tr_y = rt.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    // right
                    rt_new.bl_x = temp.tr_x;
                    rt_new.tr_x = rt.tr_x;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                }
            }
            // bottom left (rt.bl_x >= temp.bl_x && rt.bl_y >= temp.bl_y && rt.tr_x > temp.tr_x && rt.tr_y > temp.tr_y)
            else if (condition == 3) {
                // two new rects
                if (temp.tr_x - rt.bl_x > temp.tr_y - rt.bl_y) {
                    // top
                    rt_new.bl_x = rt.bl_x;
                    rt_new.bl_y = temp.tr_y;
                    rt_new.tr_x = rt.tr_x;
                    rt_new.tr_y = rt.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    // bottom
                    rt_new.bl_x = temp.tr_x;
                    rt_new.bl_y = rt.bl_y;
                    rt_new.tr_x = rt.tr_x;
                    rt_new.tr_y = temp.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                }
                else {
                    // left
                    rt_new.bl_x = rt.bl_x;
                    rt_new.bl_y = temp.tr_y;
                    rt_new.tr_x = temp.tr_x;
                    rt_new.tr_y = rt.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    // right
                    rt_new.bl_x = temp.tr_x;
                    rt_new.bl_y = rt.bl_y;
                    rt_new.tr_x = rt.tr_x;
                    rt_new.tr_y = rt.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                }
            }
            // right middle (rt.bl_x < temp.bl_x && rt.bl_y < temp.bl_y && rt.tr_x <= temp.tr_x && rt.tr_y > temp.tr_y)
            else if (condition == 4) {
                // three new rects
                if (rt.tr_x - temp.bl_x > temp.tr_y - temp.bl_y) {
                    // top
                    rt_new.bl_x = rt.bl_x;
                    rt_new.bl_y = temp.tr_y;
                    rt_new.tr_x = rt.tr_x;
                    rt_new.tr_y = rt.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    // bottom
                    rt_new.bl_y = rt.bl_y;
                    rt_new.tr_y = temp.bl_y;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    // left
                    rt_new.bl_x = rt.bl_x;
                    rt_new.bl_y = temp.bl_y;
                    rt_new.tr_x = temp.bl_x;
                    rt_new.tr_y = temp.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                }
                else {
                    // top
                    rt_new.bl_x = temp.bl_x;
                    rt_new.bl_y = temp.tr_y;
                    rt_new.tr_x = rt.tr_x;
                    rt_new.tr_y = rt.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    // bottom
                    rt_new.bl_y = rt.bl_y;
                    rt_new.tr_y = temp.bl_y;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    // left
                    rt_new.bl_x = rt.bl_x;
                    rt_new.bl_y = rt.bl_y;
                    rt_new.tr_x = temp.bl_x;
                    rt_new.tr_y = rt.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                }
            }
            // horizontal middle (rt.bl_x >= temp.bl_x && rt.bl_y < temp.bl_y && rt.tr_x <= temp.tr_x && rt.tr_y > temp.tr_y)
            else if (condition = 5) {
                // two new rects
                // top
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = temp.tr_y;
                rt_new.tr_x = rt.tr_x;
                rt_new.tr_y =  rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                    temp_rts.emplace_back(rt_new);
                // bottom
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_y = temp.bl_y;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                    temp_rts.emplace_back(rt_new);
                    
            }
            // bottom right (rt.bl_x < temp.bl_x && rt.bl_y >= temp.bl_y && rt.tr_x <= temp.tr_x && rt.tr_y > temp.tr_y)
            else if (condition == 6) {
                // two new rects
                if (rt.tr_x - temp.bl_x > temp.tr_y - rt.bl_y) {
                    // top
                    rt_new.bl_x = rt.bl_x;
                    rt_new.bl_y = temp.tr_y;
                    rt_new.tr_x = rt.tr_x;
                    rt_new.tr_y = rt.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    // bottom
                    rt_new.bl_x = rt.bl_x;
                    rt_new.bl_y = rt.bl_y;
                    rt_new.tr_x = temp.bl_x;
                    rt_new.tr_y = temp.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                }
                else {
                    // left
                    rt_new.bl_x = rt.bl_x;
                    rt_new.bl_y = rt.bl_y;
                    rt_new.tr_x = temp.bl_x;
                    rt_new.tr_y = rt.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    // right
                    rt_new.bl_x = temp.bl_x;
                    rt_new.bl_y = temp.tr_y;
                    rt_new.tr_x = rt.tr_x;
                    rt_new.tr_y = rt.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                }
            }
            // bottom (rt.bl_x >= temp.bl_x && rt.bl_y >= temp.bl_y && rt.tr_x <= temp.tr_x && rt.tr_y > temp.tr_y)
            else if (condition == 7) {
                // one new rect
                // top
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = temp.tr_y;
                rt_new.tr_x = rt.tr_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                    temp_rts.emplace_back(rt_new);
            }
            // top middle (rt.bl_x < temp.bl_x && rt.bl_y < temp.bl_y && rt.tr_x > temp.tr_x && rt.tr_y <= temp.tr_y)
            else if (condition == 8) {
                // three new rects
                if (temp.tr_x - temp.bl_x > rt.tr_y - temp.bl_y) {
                    // left
                    rt_new.bl_x = rt.bl_x;
                    rt_new.bl_y = temp.bl_y;
                    rt_new.tr_x = temp.bl_x;
                    rt_new.tr_y = rt.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    // right
                    rt_new.bl_x = temp.tr_x;
                    rt_new.tr_x = rt.tr_x;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    // bottom
                    rt_new.bl_x = rt.bl_x;
                    rt_new.bl_y = rt.bl_y;
                    rt_new.tr_x = rt.tr_x;
                    rt_new.tr_y = temp.bl_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                }
                else {
                    // left
                    rt_new.bl_x = rt.bl_x;
                    rt_new.bl_y = rt.bl_y;
                    rt_new.tr_x = temp.bl_x;
                    rt_new.tr_y = rt.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    // right
                    rt_new.bl_x = temp.tr_x;
                    rt_new.tr_x = rt.tr_x;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    // bottom
                    rt_new.bl_x = temp.bl_x;
                    rt_new.bl_y = rt.bl_y;
                    rt_new.tr_x = temp.tr_x;
                    rt_new.tr_y = temp.bl_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                }
            }
            // top left (rt.bl_x >= temp.bl_x && rt.bl_y < temp.bl_y && rt.tr_x > temp.tr_x && rt.tr_y <= temp.tr_y)
            else if (condition == 9) {
                // two new rects
                if (temp.tr_x - rt.bl_x > rt.tr_y - temp.bl_y) {
                    // top
                    rt_new.bl_x = temp.tr_x;
                    rt_new.bl_y = temp.bl_y;
                    rt_new.tr_x = rt.tr_x;
                    rt_new.tr_y = rt.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    // bottom
                    rt_new.bl_x = rt.bl_x;
                    rt_new.bl_y = rt.bl_y;
                    rt_new.tr_x = rt.tr_x;
                    rt_new.tr_y = temp.bl_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                }
                else {
                    // left
                    rt_new.bl_x = rt.bl_x;
                    rt_new.bl_y = rt.bl_y;
                    rt_new.tr_x = temp.tr_x;
                    rt_new.tr_y = temp.bl_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    //right
                    rt_new.bl_x = temp.tr_x;
                    rt_new.bl_y = rt.bl_y;
                    rt_new.tr_x = rt.tr_x;
                    rt_new.tr_y = rt.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                }
            }
            // vertical middle (rt.bl_x < temp.bl_x && rt.bl_y >= temp.bl_y && rt.tr_x > temp.tr_x && rt.tr_y <= temp.tr_y)
            else if (condition == 10) {
                // two new rects
                // left
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_x = temp.bl_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                    temp_rts.emplace_back(rt_new);
                // right
                rt_new.bl_x = temp.tr_x;
                rt_new.tr_x = rt.tr_x;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                    temp_rts.emplace_back(rt_new);
            }
            // left (rt.bl_x >= temp.bl_x && rt.bl_y >= temp.bl_y && rt.tr_x > temp.tr_x && rt.tr_y <= temp.tr_y)
            else if (condition == 11) {
                // one new rect
                // right
                rt_new.bl_x = temp.tr_x;
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_x = rt.tr_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                    temp_rts.emplace_back(rt_new);
            }
            // top right (rt.bl_x < temp.bl_x && rt.bl_y < temp.bl_y && rt.tr_x <= temp.tr_x && rt.tr_y <= temp.tr_y)
            else if (condition == 12) {
                // two new rects
                if (rt.tr_x - temp.bl_x > rt.tr_y - temp.bl_y) {
                    // top
                    rt_new.bl_x = rt.bl_x;
                    rt_new.bl_y = temp.bl_y;
                    rt_new.tr_x = temp.bl_x;
                    rt_new.tr_y = rt.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    // bottom
                    rt_new.bl_x = rt.bl_x;
                    rt_new.bl_y = rt.bl_y;
                    rt_new.tr_x = rt.tr_x;
                    rt_new.tr_y = temp.bl_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                }
                else {
                    // left
                    rt_new.bl_x = rt.bl_x;
                    rt_new.bl_y = rt.bl_y;
                    rt_new.tr_x = temp.bl_x;
                    rt_new.tr_y = rt.tr_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                    // right
                    rt_new.bl_x = temp.bl_x;
                    rt_new.bl_y = rt.bl_y;
                    rt_new.tr_x = rt.tr_x;
                    rt_new.tr_y = temp.bl_y;
                    rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                    rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                    if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                        temp_rts.emplace_back(rt_new);
                }
            }
            // top (rt.bl_x >= temp.bl_x && rt.bl_y < temp.bl_y && rt.tr_x <= temp.tr_x && rt.tr_y <= temp.tr_y)
            else if (condition == 13) {
                // one new rect
                // bottom
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_x = rt.tr_x;
                rt_new.tr_y = temp.bl_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                    temp_rts.emplace_back(rt_new);
            }
            // right (rt.bl_x < temp.bl_x && rt.bl_y >= temp.bl_y && rt.tr_x <= temp.tr_x && rt.tr_y <= temp.tr_y)
            else if (condition == 14) {
                // one new rect
                // left
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_x = temp.bl_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                    temp_rts.emplace_back(rt_new);
            }
        }

        rts.swap(temp_rts);
    }

    sort(rts.begin(), rts.end(), [](const Rect &r1, const Rect &r2) {
        return r1.width_x * r1.width_y  > r2.width_x * r2.width_y;
    });
}

Rect FindMaxSpace(const vector<int> &qw, const int min_width, const int max_width, const int min_space)
{
    Rect rt = {.bl_x = -1, .bl_y = -1, .tr_x = -1, .tr_y = -1, .width_x = -1, .width_y = -1};
    long long max_area = 0;
    long long temp_area = (long long)stride * stride;
    const int actual_min_width = min_width + 2 * min_space;
    const int actual_max_width = max_width + 2 * min_space;

    vector<int> wl(stride, 0);
    vector<int> wr(stride, 0);
    vector<int> h(stride, 0);
    vector<int> l(stride, 0);
    vector<int> r(stride, 0);

    for (int y = 0; y < stride; y++) {
        // search how far it can extend on left
        for (int x = 0; x < stride; x++) {
            if (!qw[y * stride + x]) {
                if (x == 0)
                    wl[0] = 1;
                else
                    wl[x] = wl[x - 1] + 1;
            }
            else
                wl[x] = 0;
        }
        // search how far it can extend on right
        for (int x = stride - 1; x >= 0; x--) {
            if (!qw[y * stride + x]) {
                if (x == stride - 1)
                    wr[stride - 1] = 1;
                else
                    wr[x] = wr[x + 1] + 1;
            }
            else
                wr[x] = 0;
        }
        // search how far it can extend on top
        for (int x = 0; x < stride; x++) {
            if (!qw[y * stride + x])
                h[x]++;
            else
                h[x] = 0;
        }
        // search how far it can extend on left after reaching top
        for (int x = 0; x < stride; x++) {
            if (l[x] == 0)
                l[x] = wl[x];
            else
                l[x] = min(l[x], wl[x]);
        }
        // search how far it can extend on right after reaching top
        for (int x = 0; x < stride; x++) {
            if (r[x] == 0)
                r[x] = wr[x];
            else
                r[x] = min(r[x], wr[x]);
        }

        // search for the smallest matching area
        for (int x = 0; x < stride; x++) {
            int width1 = l[x] + r[x] - 1;
            int width2 = h[x];
            // no constraint on max_width because empty space may always be larger than max_width
            if (width1 >= actual_min_width && width2 >= actual_min_width) {
                long long area = (long long)(width1 - 2 * min_space) * (width2 - 2 * min_space);
                if (area > max_area) {
                    rt.bl_x = x - l[x] + 1 + min_space;
                    rt.bl_y = y - h[x] + 1 + min_space;
                    rt.tr_x = x + r[x] - 1 - min_space;
                    rt.tr_y = y - min_space;
                    max_area = area;
                }
            }
        }
    }

    rt.width_x = rt.tr_x - rt.bl_x;
    rt.width_y = rt.tr_y - rt.bl_y;

    return rt;
}

void AddMetalFill(const Rect rt, const int layer)
{
    Layout metal_fill;
    metal_fill.id = ++total_metals;
    metal_fill.bl_x = rt.bl_x;
    metal_fill.bl_y = rt.bl_y;
    metal_fill.tr_x = rt.tr_x;
    metal_fill.tr_y = rt.tr_y;
    metal_fill.net_id = 0;
    metal_fill.layer = layer;
    metal_fill.type = 3; // Fill
    metal_fill.isCritical = false;

    layouts.emplace_back(metal_fill);
}

long long ShrinkMetalFill(Rect rt, long long target_area, const int min_width, const int layer)
{
    int x_width = rt.width_x;
    int y_width = rt.width_y;

    int offset = 10;
    int shrink_width;
    while (x_width >= min_width && y_width >= min_width && (long long)x_width * y_width > target_area) {
        // shrink at larger width
        shrink_width = x_width < y_width ? 1 : 0;
        x_width = shrink_width ? x_width : x_width - offset;
        y_width = shrink_width ? y_width - offset : y_width;
    }
    x_width = shrink_width ? x_width : x_width + offset;
    y_width = shrink_width ? y_width + offset : y_width;

    int x_offset = (rt.width_x - x_width) / 2;
    int y_offset = (rt.width_y - y_width) / 2;
    rt.bl_x += x_offset;
    rt.bl_y += y_offset;
    rt.tr_x -= x_offset;
    rt.tr_y -= y_offset;
    rt.width_x = x_width;
    rt.width_y = y_width;
    printf(" %d*%d=%lld ", rt.width_x, rt.width_y, (long long)rt.width_x * rt.width_y);

    AddMetalFill(rt, layer);
    return (long long)rt.width_x * rt.width_y;
}

long long DivideMetalFill(Rect rt, long long target_area,
                          const int min_width, const int max_width, const int min_space, const int layer)
{
    vector<Rect> rts;
    int x_width = rt.width_x;
    int y_width = rt.width_y;
    int max_width_padding = max_width + min_space;

    Rect r = {.bl_x = -1, .bl_y = -1, .tr_x = -1, .tr_y = -1, .width_x =  max_width, .width_y =  max_width};
    int x_max_count = x_width / max_width_padding;
    int y_max_count = y_width / max_width_padding;
    // x, y = max width
    for (int y_count = 0; y_count < y_max_count; y_count++) {
        r.bl_y = rt.bl_y + y_count * max_width_padding;
        r.tr_y = r.bl_y + max_width;
        for (int x_count = 0; x_count < x_max_count; x_count++) {
            r.bl_x = rt.bl_x + x_count * max_width_padding;
            r.tr_x = r.bl_x + max_width;
            rts.emplace_back(r);
        }
    }

    int x_width_rest = x_width % max_width_padding;
    int y_width_rest = y_width % max_width_padding;
    // y = max width
    if (x_width_rest != 0 && x_width_rest >= min_width) {
        r.bl_x = rt.bl_x + x_max_count * max_width_padding;
        r.tr_x = r.bl_x + x_width_rest;
        r.width_x = x_width_rest;
        r.width_y = max_width;
        for (int y_count = 0; y_count < y_max_count; y_count++) {
            r.bl_y = rt.bl_y + y_count * max_width_padding;
            r.tr_y = r.bl_y + max_width;
            rts.emplace_back(r);
        }
    }
    // x = max width
    if (y_width_rest != 0 && y_width_rest >= min_width) {
        r.bl_y = rt.bl_y + y_max_count * max_width_padding;
        r.tr_y = r.bl_y + y_width_rest;
        r.width_y = y_width_rest;
        r.width_x = max_width;
        for (int x_count = 0; x_count < x_max_count; x_count++) {
            r.bl_x = rt.bl_x + x_count * max_width_padding;
            r.tr_x = r.bl_x + max_width;
            rts.emplace_back(r);
        }
    }

    // bottom right
    if (x_width_rest != 0 && x_width_rest >= min_width && y_width_rest != 0 && y_width_rest >= min_width) {
        r.bl_x = rt.bl_x + x_max_count * max_width_padding;
        r.tr_x = r.bl_x + x_width_rest;
        r.bl_y = rt.bl_y + y_max_count * max_width_padding;
        r.tr_y = r.bl_y + y_width_rest;
        r.width_x = x_width_rest;
        r.width_y = y_width_rest;
        rts.emplace_back(r);
    }

    // try to fill no larger than target area
    long long total_fill_area = 0;
    int size = rts.size();
    int i = 0;
    for (i = 0; i < size; i++) {
        long long rect_area = (long long)rts[i].width_x * rts[i].width_y;
        if (total_fill_area + rect_area > target_area)
            break;
        printf(" (%d, %d, %d, %d) ", rts[i].bl_x, rts[i].bl_y, rts[i].tr_x, rts[i].tr_y);
        AddMetalFill(rts[i], layer);
        total_fill_area += rect_area;
    }
    printf(" %lld ", total_fill_area);
    // if break, try to shrink
    if (i != size) {
        printf(" shrink:");
        total_fill_area += ShrinkMetalFill(rts[i], target_area - total_fill_area, min_width, layer);
    }

    printf(" %lld ", total_fill_area);
    return total_fill_area;
}

long long MinMetalFill(Rect rt, const int min_width, const int layer)
{
    int x_offset = (rt.width_x - min_width) / 2;
    int y_offset = (rt.width_y - min_width) / 2;
    rt.bl_x += x_offset;
    rt.bl_y += y_offset;
    rt.tr_x -= x_offset;
    rt.tr_y -= y_offset;
    rt.width_x = min_width;
    rt.width_y = min_width;

    return (long long)rt.width_x * rt.width_y;
}

long long UpdateMetalFill(const Rect rt, const int layer)
{
    AddMetalFill(rt, layer);
    return (long long)rt.width_x * rt.width_y;
}

void FillMetalRandomly()
{
    for (int layer = 1; layer <= total_layers; layer++) {
        vector<Window> &ws = windows[layer - 1];
        vector<QuarterWindow> &qws = quarter_windows[layer - 1];
        const Rule &r = rules[layer - 1];
        // min quarter window area
        long long min_area = (long long)stride * stride * r.min_density;
        // max, min metal fill area
        long long min_metal_fill = (long long)r.min_width * r.min_width;
        long long max_metal_fill = (long long)r.max_fill_width * r.max_fill_width;

        int max_qwindows = qwindow_x * qwindow_y;
        for (int qw_idx = 0; qw_idx < max_qwindows; qw_idx++) {
            QuarterWindow &qw = qws[qw_idx];
            long long target_area = min_area - qw.area;
            printf("%d %lld\n", qw_idx, qw.area);

            vector<Rect> rts;
            FindSpace(rts, qw, r.min_width, r.min_space);

            for (Rect metal_fill : rts) {
                metal_fill.bl_x += r.min_space;
                metal_fill.bl_y += r.min_space;
                metal_fill.tr_x -= r.min_space;
                metal_fill.tr_y -= r.min_space;
                metal_fill.width_x = metal_fill.tr_x -  metal_fill.bl_x;
                metal_fill.width_y = metal_fill.tr_y -  metal_fill.bl_y;
                long long metal_fill_area = (long long)metal_fill.width_x * metal_fill.width_y;
                printf("  %lld %lld\n", target_area, metal_fill_area);

                if (target_area < min_metal_fill) {
                    printf("    min\n");
                    target_area -= MinMetalFill(metal_fill, r.min_width, layer);
                }
                else if (target_area <= max_metal_fill) {
                    if (target_area >= metal_fill_area) {
                        printf("    update\n");
                        target_area -= UpdateMetalFill(metal_fill, layer);
                    }
                    else {
                        printf("    shrink: ");
                        target_area -= ShrinkMetalFill(metal_fill, target_area, r.min_width, layer);
                        printf("\n");
                    }
                }
                else {
                    if (metal_fill_area <= max_metal_fill) {
                        printf("    update\n");
                        target_area -= UpdateMetalFill(metal_fill, layer);
                    }
                    else {
                        printf("    divide: ");
                        target_area -= DivideMetalFill(metal_fill, target_area,
                                                       r.min_width, r.max_fill_width, r.min_space, layer);
                        printf("\n");
                    }
                }

                if (target_area <= 0)
                    break;
            }

            if (target_area > 0) {
                printf("[Error] target area remaining after metal fill\n");
                exit(1);
            }
        }
    }
}

void OutputLayout()
{
    ofstream file(path + "circuit_metal-fill.cut");

    for (Layout temp : metal_fill_layouts) {
        file << temp.id << " " << temp.bl_x + cb.bl_x << " " << temp.bl_y + cb.bl_y << " "
             << temp.tr_x + cb.bl_x << " " << temp.tr_y + cb.bl_y << " "
             << temp.net_id << " " << temp.layer << " Fill\n";
    }

    file.close();
}

void free_memory()
{
    // int total_layers = metals.size();
    // for (int i = 0; i < total_layers; i++) {
    //     Metal *metal_x = metals[i];
    //     Metal *metal_y = metals[i];
    //     Metal *metal_x_next = NULL;
    //     Metal *metal_y_next = NULL;
    //     while (metal_x != NULL) {
    //         metal_x_next = metal_x->next_x;
    //         while (metal_y != NULL) {
    //             metal_y_next = metal_y->next_y;
    //             delete metal_y;
    //             metal_y = metal_y_next;
    //         }
    //         metal_x = metal_x_next;
    //         metal_y = metal_x;
    //     }
    //     metals[i] = NULL;
    // }
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("Usage: ./<executable> <config file>\n");
        exit(1);
    }
    config_file = argv[1];
    ReadConfig();
    ReadCircuit();
    ReadProcess();
    ReadRule();

    AnalyzeDensity();
    
    // initialize cap table (symmetric lower traingular matrix)
    // long long total_map_size = total_metals * (total_metals + 1) / 2;
    // for (long long i = 0; i < total_map_size; i++)
    //     cap[i] = 0;
    // CalculateAreaCapacitance();
    // CalculateLateralCapacitance();
    // CalculateFringeCapacitance();

    FillMetalRandomly();

    // free_memory();

    return 0;
}
