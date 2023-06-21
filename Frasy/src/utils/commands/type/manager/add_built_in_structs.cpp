/**
 * @file    add_built_in_structs.cpp
 * @author  Paul Thomas
 * @date    2023-02-16
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
#include "../../built_in/command_info/reply.h"
#include "../../built_in/identify/reply.h"
#include "../../built_in/status/reply.h"
#include "../../description/value.h"
#include "manager.h"

namespace Frasy::Type
{
void Manager::AddBuiltInStructs()
{
    //<editor-fold desc="Enum::Field">
    using Type::Enum;
    Enum::Field::Manager::id = AddStruct(Struct {
      .Name = std::string(Enum::Field::Manager::name),
      .Fields =
        {
          Struct::Field {
            .Name        = std::string(Enum::Field::Manager::Name::name),
            .Type        = Enum::Field::Manager::Name::type,
            .Description = std::string(Enum::Field::Manager::Name::description),
          },
          Struct::Field {
            .Name        = std::string(Enum::Field::Manager::Value::name),
            .Type        = Enum::Field::Manager::Value::type,
            .Description = std::string(Enum::Field::Manager::Value::description),
          },
          Struct::Field {
            .Name        = std::string(Enum::Field::Manager::Description::name),
            .Type        = Enum::Field::Manager::Description::type,
            .Description = std::string(Enum::Field::Manager::Description::description),
          },
        },
      .Description = std::string(Enum::Field::Manager::description),
    });
    //</editor-fold>

    //<editor-fold desc="Enum">
    using Type::Enum;
    Enum::Manager::id =
      AddStruct(
        Struct {
          .Name = std::string(Enum::Manager::name),
          .Fields =
            {
              Struct::Field {
                .Name        = std::string(Enum::Manager::Name::name),
                .Type        = Enum::Manager::Name::type,
                .Description = std::string(Enum::Manager::Name::description),
              },
              Struct::Field {
                .Name        = std::string(Enum::Manager::Fields::name),
                .Type        = Enum::Manager::Fields::type(),
                .Count       = Enum::Manager::Fields::count,
                .Description = std::string(Enum::Manager::Fields::description),
              },
              Struct::Field {
                .Name        = std::string(Enum::Manager::Description::name),
                .Type        = Enum::Manager::Description::type,
                .Description = std::string(Enum::Manager::Description::description),
              },
              Struct::Field {
                .Name        = std::string(Enum::Manager::UnderlyingType::name),
                .Type        = Enum::Manager::UnderlyingType::type,
                .Description = std::string(Enum::Manager::UnderlyingType::description),
              },
            },
          .Description = std::string(Enum::Manager::description),
        });
    //</editor-fold>

    //<editor-fold desc="Struct::Field">
    using Type::Struct;
    Struct::Field::Manager::id =
      AddStruct(
        Struct {
          .Name = std::string(Struct::Field::Manager::name),
          .Fields =
            {
              Struct::Field {
                .Name        = std::string(Struct::Field::Manager::Name::name),
                .Type        = Struct::Field::Manager::Name::type,
                .Description = std::string(Struct::Field::Manager::Name::description),
              },
              Struct::Field {
                .Name        = std::string(Struct::Field::Manager::Type::name),
                .Type        = Struct::Field::Manager::Type::type,
                .Description = std::string(Struct::Field::Manager::Type::description),
              },
              Struct::Field {
                .Name        = std::string(Struct::Field::Manager::Count::name),
                .Type        = Struct::Field::Manager::Count::type,
                .Description = std::string(Struct::Field::Manager::Count::description),
              },
              Struct::Field {
                .Name        = std::string(Struct::Field::Manager::Description::name),
                .Type        = Struct::Field::Manager::Description::type,
                .Description = std::string(Struct::Field::Manager::Description::description),
              },
            },
          .Description = std::string(Struct::Field::Manager::description),
        });
    //</editor-fold>

    //<editor-fold desc="Struct">
    using Type::Struct;
    Struct::Manager::id = AddStruct(Struct {
      .Name = std::string(Struct::Manager::name),
      .Fields =
        {
          Struct::Field {
            .Name        = std::string(Struct::Manager::Name::name),
            .Type        = Struct::Manager::Name::type,
            .Description = std::string(Struct::Manager::Name::description),
          },
          Struct::Field {
            .Name        = std::string(Struct::Manager::Fields::name),
            .Type        = Struct::Manager::Fields::type(),
            .Count       = Struct::Manager::Fields::count,
            .Description = std::string(Struct::Manager::Fields::description),
          },
          Struct::Field {
            .Name        = std::string(Struct::Manager::Description::name),
            .Type        = Struct::Manager::Description::type,
            .Description = std::string(Struct::Manager::Description::description),
          },
        },
      .Description = std::string(Struct::Manager::description),
    });
    //</editor-fold>

    //<editor-fold desc="Value">
    using Frasy::Actions::Value;
    using Type::Struct;
    Value::Manager::id =
      AddStruct(
        Struct {
          .Name = std::string(Value::Manager::name),
          .Fields =
            {
              Struct::Field {
                .Name        = std::string(Value::Manager::Pos::name),
                .Type        = Value::Manager::Pos::type,
                .Description = std::string(Value::Manager::Pos::description),
              },
              Struct::Field {
                .Name        = std::string(Value::Manager::Name::name),
                .Type        = Value::Manager::Name::type,
                .Description = std::string(Value::Manager::Name::description),
              },
              Struct::Field {
                .Name        = std::string(Value::Manager::Help::name),
                .Type        = Value::Manager::Help::type,
                .Description = std::string(Value::Manager::Help::description),
              },
              Struct::Field {
                .Name        = std::string(Value::Manager::Alias::name),
                .Type        = Value::Manager::Alias::type,
                .Description = std::string(Value::Manager::Alias::description),
              },
              Struct::Field {
                .Name        = std::string(Value::Manager::Type::name),
                .Type        = Value::Manager::Type::type,
                .Description = std::string(Value::Manager::Type::description),
              },
              Struct::Field {
                .Name        = std::string(Value::Manager::Count::name),
                .Type        = Value::Manager::Count::type,
                .Description = std::string(Value::Manager::Count::description),
              },
              Struct::Field {
                .Name        = std::string(Value::Manager::Min::name),
                .Type        = Value::Manager::Min::type,
                .Description = std::string(Value::Manager::Min::description),
              },
              Struct::Field {
                .Name        = std::string(Value::Manager::Max::name),
                .Type        = Value::Manager::Max::type,
                .Description = std::string(Value::Manager::Max::description),
              },
            },
          .Description = std::string(Value::Manager::description),
        });
    //</editor-fold>

    //<editor-fold desc="Type::BasicInfo">
    using Type::BasicInfo;
    BasicInfo::Manager::id = AddStruct(Struct {
      .Name = std::string(BasicInfo::Manager::name),
      .Fields =
        {
          Struct::Field {
            .Name        = std::string(BasicInfo::Manager::Id::name),
            .Type        = BasicInfo::Manager::Id::type,
            .Description = std::string(BasicInfo::Manager::Id::description),
          },
          Struct::Field {
            .Name        = std::string(BasicInfo::Manager::Name::name),
            .Type        = BasicInfo::Manager::Name::type,
            .Description = std::string(BasicInfo::Manager::Name::description),
          },
        },
      .Description = std::string(BasicInfo::Manager::description),
    });
    //</editor-fold>

    //<editor-fold desc="CommandInfo::Reply">
    {
        using namespace Frasy::Actions::CommandInfo;
        // Example for using Struct with commands
        Reply::Manager::id = AddStruct(Struct()
                                         .SetName(std::string(Reply::Manager::name))
                                         .SetDescription(std::string(Reply::Manager::description))
                                         .SetFields({
                                           Struct::Field()
                                             .SetName(std::string(Reply::Manager::Name::name))
                                             .SetType(Reply::Manager::Name::type)
                                             .SetDescription(std::string(Reply::Manager::Name::description)),
                                           Struct::Field()
                                             .SetName(std::string(Reply::Manager::Help::name))
                                             .SetType(Reply::Manager::Help::type)
                                             .SetDescription(std::string(Reply::Manager::Help::description)),
                                           Struct::Field()
                                             .SetName(std::string(Reply::Manager::Id::name))
                                             .SetType(Reply::Manager::Help::type)
                                             .SetDescription(std::string(Reply::Manager::Id::description)),
                                           Struct::Field()
                                             .SetName(std::string(Reply::Manager::Parameters::name))
                                             .SetType(Reply::Manager::Parameters::type())
                                             .SetDescription(std::string(Reply::Manager::Parameters::description)),
                                           Struct::Field()
                                             .SetName(std::string(Reply::Manager::Returns::name))
                                             .SetType(Reply::Manager::Returns::type())
                                             .SetDescription(std::string(Reply::Manager::Returns::description)),
                                           Struct::Field()
                                             .SetName(std::string(Reply::Manager::Alias::name))
                                             .SetType(Reply::Manager::Alias::type)
                                             .SetDescription(std::string(Reply::Manager::Alias::description)),
                                         }));
    }
    //</editor-fold>

    //<editor-fold desc="Identify::Reply">
    {
        using namespace Frasy::Actions::Identify;
        Reply::Manager::id =
          AddStruct(
            Struct {
              .Name = std::string(Reply::Manager::name),
              .Fields =
                {
                  Struct::Field {
                    .Name        = std::string(Reply::Manager::Uuid::name),
                    .Type        = Reply::Manager::Uuid::type,
                    .Count       = Reply::Manager::Uuid::count,
                    .Description = std::string(Reply::Manager::Uuid::description),
                  },
                  Struct::Field {
                    .Name        = std::string(Reply::Manager::Id::name),
                    .Type        = Reply::Manager::Id::type,
                    .Description = std::string(Reply::Manager::Id::description),
                  },
                  Struct::Field {
                    .Name        = std::string(Reply::Manager::Version::name),
                    .Type        = Reply::Manager::Version::type,
                    .Count       = Reply::Manager::Version::count,
                    .Description = std::string(Reply::Manager::Version::description),
                  },
                  Struct::Field {
                    .Name        = std::string(Reply::Manager::PrjName::name),
                    .Type        = Reply::Manager::PrjName::type,
                    .Count       = Reply::Manager::PrjName::count,
                    .Description = std::string(Reply::Manager::PrjName::description),
                  },
                  Struct::Field {
                    .Name        = std::string(Reply::Manager::BuildTime::name),
                    .Type        = Reply::Manager::BuildTime::type,
                    .Count       = Reply::Manager::BuildTime::count,
                    .Description = std::string(Reply::Manager::BuildTime::description),
                  },
                  Struct::Field {
                    .Name        = std::string(Reply::Manager::BuildDate::name),
                    .Type        = Reply::Manager::BuildDate::type,
                    .Count       = Reply::Manager::BuildDate::count,
                    .Description = std::string(Reply::Manager::BuildDate::description),
                  },
                },
            });
    }
    //</editor-fold>

    //<editor-fold desc="Status::Reply">
    using Frasy::Actions::Status::Reply;
    Reply::Manager::id = AddStruct(Struct {
      .Name = std::string(Reply::Manager::name),
      .Fields =
        {
          Struct::Field {
            .Name        = std::string(Reply::Manager::Message::name),
            .Type        = Reply::Manager::Message::type,
            .Description = std::string(Reply::Manager::Message::description),
          },
          Struct::Field {
            .Name        = std::string(Reply::Manager::Code::name),
            .Type        = Reply::Manager::Code::type(),
            .Description = std::string(Reply::Manager::Code::description),
          },
        },
      .Description = std::string(Reply::Manager::description),
    });
    //</editor-fold>
}
}    // namespace Frasy::Type
