#include <algorithm>
#include <cmd_circle.hpp>
#include <cmd_comment.hpp>
#include <cmd_move_at.hpp>
#include <cmd_point.hpp>
#include <cmd_plane.hpp>
#include <cmd_report.hpp>
#include <cmm_wrap.hpp>
#include <globals.hpp>
#include <gui.hpp>
#include <imgui.h>
#include <iostream>
#include <stdio.h> // snprintf
#include <sstream>
#include <ios>
#include <mutex>

static std::mutex gui_mutex = {};

void gui::draw(int width, int height)
{
    // True whenever there is a popup, or we are running
    // the program. Changes on the fly is a cool thing but
    // pretty much an unnececssary one in this case.
    bool allow_changes = !globals::is_running_program;

    // True when this frame has done something
    // with the selected command so we don't
    // automatically de-select it at the end.
    bool selection_active = false;

    if(!globals::popups.empty()) {
        globals::is_running_program = false;
        ImGui::OpenPopup(globals::popups.back().title.c_str());
    }

    if(ImGui::BeginMainMenuBar()) {
        if(ImGui::BeginMenu("File")) {
            ImGui::MenuItem("New", "Ctrl+N");
            ImGui::MenuItem("Open", "Ctrl+O");
            ImGui::MenuItem("Save", "Ctrl+S");
            ImGui::MenuItem("Save As", "Ctrl+Shift+S");
            ImGui::Separator();

            if(ImGui::MenuItem("Exit"))
                glfwSetWindowShouldClose(globals::window, true);
            ImGui::EndMenu();
        }

        if(globals::machine->get_cap(CmmCap::DebugCMM)) {
            if(ImGui::BeginMenu("Debug")) {
                double temp;

                temp = globals::machine->get_opt(CmmOpt::DebugRandomness);
                if(ImGui::InputDouble("Randomness", &temp)) {
                    globals::machine->set_opt(CmmOpt::DebugRandomness, temp);
                }

                temp = globals::machine->get_opt(CmmOpt::DebugTimeDelay);
                if(ImGui::InputDouble("Time Delay", &temp)) {
                    globals::machine->set_opt(CmmOpt::DebugTimeDelay, temp);
                }

                ImGui::EndMenu();
            }
        }

        ImGui::Separator();

        const bool is_running = globals::is_running_program;
        const bool is_busy = globals::machine->is_busy();
        const char *string_status = "[IDLE // STOP]";
        if(is_busy && is_running)
            string_status = "[BUSY // EXEC]";
        else if(is_busy)
            string_status = "[BUSY // STOP]";
        else if(is_running)
            string_status = "[IDLE // EXEC]";
        ImGui::MenuItem(globals::machine->get_ident(), nullptr, nullptr, false);
        ImGui::MenuItem(string_status, nullptr, nullptr, false);

        ImGui::Separator();

        ImGui::EndMainMenuBar();
    }

    if(ImGui::Begin("Commands"), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize) {
        ImGui::SetWindowPos(ImVec2{16.0f, 32.0f}, ImGuiCond_Always);
        ImGui::SetWindowSize(ImVec2{static_cast<float>(width) - 32.0f, static_cast<float>(height) - 48.0f}, ImGuiCond_Always);


        if(ImGui::Button("Run")) {
            if(globals::current == globals::commands.end())
                globals::current = globals::commands.begin();
            globals::is_running_program = true;

            for(auto it : globals::commands) {
                if(!it->validate()) {
                    globals::popups.push(Popup {
                        .title = "Validation",
                        .content = std::string{"Command "} + std::string{it->get_name()} + std::string{ "failed validation check!"},
                        .abortable = false,
                    });
                    globals::selected_command = it;
                    globals::current = globals::commands.end();
                    globals::is_running_program = false;
                    break;
                }
            }
        }

        ImGui::SameLine();
        if(ImGui::Button("Pause")) {
            // Pause the execution, normal flow
            // can be continued by pressing Run again
            // or user can run commands step-by-step
            globals::is_running_program = false;
        }

        ImGui::SameLine();
        if(ImGui::Button("Step") && !globals::is_running_program) {
            if(globals::current == globals::commands.end())
                globals::current = globals::commands.begin();
            cmm_wrap::step();
        }

        ImGui::SameLine();
        if(ImGui::Button("Abort")) {
            // Stops the program execution at all,
            // resetting the internal program counter
            cmm_wrap::abort();
        }

        ImGui::SeparatorText("Add");

        if(ImGui::Button("Comment") && allow_changes) {
            globals::commands.push_back(new CommentCmd{});
            cmm_wrap::abort();
        }

        ImGui::SameLine();
        if(ImGui::Button("MoveAtXYZ") && allow_changes) {
            globals::commands.push_back(new MoveCmd{});
            cmm_wrap::abort();
        }

        ImGui::SameLine();
        if(ImGui::Button("Point") && allow_changes) {
            globals::commands.push_back(new PointCmd{});
            cmm_wrap::abort();
        }

        if(ImGui::Button("Plane") && allow_changes) {
            globals::commands.push_back(new PlaneCmd{});
            cmm_wrap::abort();
        }

        ImGui::SameLine();
        if(ImGui::Button("Circle") && allow_changes) {
            globals::commands.push_back(new CircleCmd{});
            cmm_wrap::abort();
        }

        if(ImGui::Button("Report") && allow_changes) {
            globals::commands.push_back(new ReportCmd{});
            cmm_wrap::abort();
        }

        ImGui::SeparatorText("Manage");

        if(ImGui::Button("Move Up") && allow_changes && !globals::commands.empty()) {
            for(auto it = globals::commands.begin() + 1; it != globals::commands.end(); ++it) {
                if(*it == globals::selected_command) {
                    std::swap(*it, *(it - 1));
                    selection_active = true;
                    cmm_wrap::abort();
                    break;
                }
            }
        }

        ImGui::SameLine();
        if(ImGui::Button("Move Down") && allow_changes && !globals::commands.empty()) {
            for(auto it = globals::commands.begin(); it != globals::commands.end() - 1; ++it) {
                if(*it == globals::selected_command) {
                    std::swap(*it, *(it + 1));
                    selection_active = true;
                    cmm_wrap::abort();
                    break;
                }
            }
        }

        if(ImGui::Button("Remove") && allow_changes && !globals::commands.empty()) {
            if(globals::selected_command) {
                delete globals::selected_command;
                globals::commands.erase(std::remove(globals::commands.begin(), globals::commands.end(), globals::selected_command), globals::commands.end());
                globals::selected_command = nullptr;
                cmm_wrap::abort();
            }
        }

        ImGui::SeparatorText("Commands");
        if(ImGui::BeginChild("CommandList")) {
            char stager[128] = {0};
            size_t command_index = 0;

            for(ICmd *it : globals::commands) {
                it->set_pcounter(command_index++);

                // ImGui breaks if the collapsing headers
                // have the same name (ie CommentCommand), so we
                // have to introduce some unique-ness to them by
                // prefixing them with a hexadecimal index.
                // I don't know a better way than to snprintf it.
                snprintf(stager, sizeof(stager), "[%04zX] %s", it->get_pcounter(), it->get_name().c_str());

                // Ensure the selected command is visible
                // while de-selected commands are not
                ImGui::SetNextItemOpen(globals::selected_command == it);

                if(ImGui::CollapsingHeader(stager)) {
                    if(allow_changes)
                        it->on_draw_imgui();
                    globals::selected_command = it;
                    selection_active = true;
                }
            }
        } ImGui::EndChild();
    } ImGui::End();

    if(!globals::popups.empty()) {
        gui_mutex.lock();

        const Popup &info = globals::popups.back();

        if(ImGui::BeginPopupModal(info.title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted(info.content.c_str());

            if(info.abortable && ImGui::Button("Abort")) {
                ImGui::CloseCurrentPopup();
                globals::popups.pop();
                cmm_wrap::abort();

                ImGui::SameLine();
            }

            if(ImGui::Button(info.abortable ? "Continue" : "OK")) {
                ImGui::CloseCurrentPopup();
                globals::popups.pop();
                globals::is_running_program = true;
            }

            ImGui::EndPopup();
        }

        gui_mutex.unlock();
    }

    if(!selection_active) {
        globals::selected_command = nullptr;
    }
}
