/**
 * @file    file.h
 * @author  Samuel Martel
 * @date    2023-05-04
 * @brief
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program. If
 * not, see <a href=https://www.gnu.org/licenses/>https://www.gnu.org/licenses/</a>.
 */

#ifndef BRIGERAD_SRC_BRIGERAD_UTILS_DIALOGS_FILE_H
#define BRIGERAD_SRC_BRIGERAD_UTILS_DIALOGS_FILE_H

#include <optional>
#include <string>
#include <vector>

namespace Brigerad::Dialogs
{
/**
 * Displays a dialog window that prompts the user to select a file name to save a file.
 * @param title Title of the dialog window. Defaults to "Save File"
 * @param defaultPathAndFile Path and filename with which the dialog window first appears.
 *                           Defaults to the current working directory and no filename.
 * @param filters List of filters to be used by the dialog. They usually take the form of "*.extension", but can be any string.
 * @param description String displayed by the dialog to describe the filters.
 * @return The path of the file that the user has selected, or nothing if the user clicked on cancel.
 */
std::optional<std::string> SaveFile(const std::string&              title              = "Save File",
                                    const std::string&              defaultPathAndFile = {},
                                    const std::vector<std::string>& filters            = {},
                                    const std::string&              description        = {});

/**
 * Displays a dialog window that prompts the user to select a file to open.
 * @param title Title of the dialog window. Defaults to "open File"
 * @param defaultPathAndFile Path and filename with which the dialog window first appears.
 *                           Defaults to the current working directory and no filename.
 * @param filters List of filters to be used by the dialog. They usually take the form of ".extension", but can be any string.
 * @param filterDescription String displayed by the dialog to describe the filters.
 * @return The path of the file that the user has selected, or nothing if the user clicked on cancel.
 */
std::optional<std::string> OpenFile(const std::string&              title              = "open File",
                                    const std::string&              defaultPathAndFile = {},
                                    const std::vector<std::string>& filters            = {},
                                    const std::string&              filterDescription  = {});

/**
 * Displays a dialog window that prompts the user to select one or more files to open.
 * @param title Title of the dialog window. Defaults to "open Files".
 * @param defaultPathAndFile Path and filename with which the dialog window first appears.
 *                           Defaults to the current working directory and no filename.
 * @param filters List of filters to be used by the dialog. They usually take the form of ".extension", but can be any string.
 * @param filterDescription String displayed by the dialog to describe the filters.
 * @return A list of the paths selected by the user, or nothing if the user clicked on cancel.
 */
std::optional<std::vector<std::string>> OpenFiles(const std::string&              title              = "open Files",
                                                  const std::string&              defaultPathAndFile = {},
                                                  const std::vector<std::string>& filters            = {},
                                                  const std::string&              filterDescription  = {});

/**
 * Displays a dialog window that prompts the user to select a directory to open.
 * @param title Title of the dialog window. Defaults to "open Directory".
 * @param defaultPathAndFile Path and filename with which the dialog window first appears.
 *                           Defaults to the current working directory and no filename.
 * @return The path of the directory that the user has selected, or nothing if the user clicked on cancel.
 */
std::optional<std::string> OpenFolder(const std::string&              title              = "open Directory",
                                      const std::string&              defaultPathAndFile = {});

}    // namespace Brigerad::Dialogs
#endif    // BRIGERAD_SRC_BRIGERAD_UTILS_DIALOGS_FILE_H
