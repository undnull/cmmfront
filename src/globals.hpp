#ifndef B9B8A19D_1E12_4686_9383_A5153E38CF69
#define B9B8A19D_1E12_4686_9383_A5153E38CF69
#include <deque>
#include <GLFW/glfw3.h>
#include <icmm.hpp>
#include <icmd.hpp>
#include <string>

namespace globals
{
extern GLFWwindow *window;
extern ICMM *machine;
extern ICmd *selected_command;
extern std::deque<ICmd *> commands;
extern std::deque<ICmd *>::iterator current;
extern bool is_running_program;
extern std::string popup_class;
extern std::string popup_text;
} // namespace globals

#endif/* B9B8A19D_1E12_4686_9383_A5153E38CF69 */
