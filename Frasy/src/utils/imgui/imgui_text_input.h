/**
 ******************************************************************************
 * @addtogroup ImguiTextInput
 * @{
 * @file    ImguiTextInput
 * @author  Client Microdata
 * @brief   Header for the ImguiTextInput module.
 *
 * @date 9/29/2020 2:35:06 PM
 *
 ******************************************************************************
 */
#ifndef _ImguiTextInput
#define _ImguiTextInput

/*****************************************************************************/
/* Includes */
#include "ImGui/imgui.h"
#include <string>

/*****************************************************************************/
/* Exported defines */


/*****************************************************************************/
/* Exported macro */


/*****************************************************************************/
/* Exported types */

struct StringInput
{
    static int MyResizeCallback(ImGuiInputTextCallbackData* data)
    {
        if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
        {
            std::string* my_str = (std::string*)data->UserData;
            IM_ASSERT(my_str->data() == data->Buf);
            my_str->resize(data->BufSize);    // NB: On resizing calls, generally data->BufSize ==
                                              // data->BufTextLen + 1
            data->Buf = my_str->data();
        }
        return 0;
    }

    // Tip: Because ImGui:: is a namespace you would typicall add your own function into the
    // namespace in your own source files. For example, you may add a function called
    // ImGui::InputText(const char* label, MyString* my_str).
    static bool Input(const char* label, std::string& my_str, ImGuiInputTextFlags flags = 0)
    {
        IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
        return ImGui::InputText(label,
                                my_str.data(),
                                (size_t)my_str.size(),
                                flags | ImGuiInputTextFlags_CallbackResize,
                                StringInput::MyResizeCallback,
                                (void*)&my_str);
    }

    static bool MultilineInput(const char*         label,
                               std::string&        my_str,
                               ImGuiInputTextFlags flags = 0)
    {
        IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
        return ImGui::InputTextMultiline(label,
                                         my_str.data(),
                                         (size_t)my_str.size(),
                                         ImVec2(0, 0),
                                         flags | ImGuiInputTextFlags_CallbackResize,
                                         StringInput::MyResizeCallback,
                                         (void*)&my_str);
    }
};

/*****************************************************************************/
/* Exported functions */


/* Have a wonderful day :) */
#endif /* _ImguiTextInput */
/**
 * @}
 */
/****** END OF FILE ******/
