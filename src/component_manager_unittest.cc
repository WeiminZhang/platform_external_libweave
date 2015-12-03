// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/component_manager.h"

#include <map>

#include <gtest/gtest.h>
#include <weave/test/unittest_utils.h>

#include "src/bind_lambda.h"
#include "src/commands/schema_constants.h"

namespace weave {

using test::CreateDictionaryValue;

namespace {

bool HasTrait(const base::DictionaryValue& comp, const std::string& trait) {
  const base::ListValue* list = nullptr;
  if (!comp.GetList("traits", &list))
    return false;
  for (const base::Value* item : *list) {
    std::string value;
    if (item->GetAsString(&value) && value == trait)
      return true;
  }
  return false;
}

}  // anonymous namespace

TEST(ComponentManager, Empty) {
  ComponentManager manager;
  EXPECT_TRUE(manager.GetTraits().empty());
  EXPECT_TRUE(manager.GetComponents().empty());
}

TEST(ComponentManager, LoadTraits) {
  ComponentManager manager;
  const char kTraits[] = R"({
    "trait1": {
      "commands": {
        "command1": {
          "minimalRole": "user",
          "parameters": {"height": {"type": "integer"}}
        }
      },
      "state": {
        "property1": {"type": "boolean"}
      }
    },
    "trait2": {
      "state": {
        "property2": {"type": "string"}
      }
    }
  })";
  auto json = CreateDictionaryValue(kTraits);
  EXPECT_TRUE(manager.LoadTraits(*json, nullptr));
  EXPECT_JSON_EQ(kTraits, manager.GetTraits());
  EXPECT_TRUE(manager.GetComponents().empty());
}

TEST(ComponentManager, LoadTraitsDuplicateIdentical) {
  ComponentManager manager;
  const char kTraits1[] = R"({
    "trait1": {
      "commands": {
        "command1": {
          "minimalRole": "user",
          "parameters": {"height": {"type": "integer"}}
        }
      },
      "state": {
        "property1": {"type": "boolean"}
      }
    },
    "trait2": {
      "state": {
        "property2": {"type": "string"}
      }
    }
  })";
  auto json = CreateDictionaryValue(kTraits1);
  EXPECT_TRUE(manager.LoadTraits(*json, nullptr));
  const char kTraits2[] = R"({
    "trait1": {
      "commands": {
        "command1": {
          "minimalRole": "user",
          "parameters": {"height": {"type": "integer"}}
        }
      },
      "state": {
        "property1": {"type": "boolean"}
      }
    },
    "trait3": {
      "state": {
        "property3": {"type": "string"}
      }
    }
  })";
  json = CreateDictionaryValue(kTraits2);
  EXPECT_TRUE(manager.LoadTraits(*json, nullptr));
  const char kExpected[] = R"({
    "trait1": {
      "commands": {
        "command1": {
          "minimalRole": "user",
          "parameters": {"height": {"type": "integer"}}
        }
      },
      "state": {
        "property1": {"type": "boolean"}
      }
    },
    "trait2": {
      "state": {
        "property2": {"type": "string"}
      }
    },
    "trait3": {
      "state": {
        "property3": {"type": "string"}
      }
    }
  })";
  EXPECT_JSON_EQ(kExpected, manager.GetTraits());
}

TEST(ComponentManager, LoadTraitsDuplicateOverride) {
  ComponentManager manager;
  const char kTraits1[] = R"({
    "trait1": {
      "commands": {
        "command1": {
          "minimalRole": "user",
          "parameters": {"height": {"type": "integer"}}
        }
      },
      "state": {
        "property1": {"type": "boolean"}
      }
    },
    "trait2": {
      "state": {
        "property2": {"type": "string"}
      }
    }
  })";
  auto json = CreateDictionaryValue(kTraits1);
  EXPECT_TRUE(manager.LoadTraits(*json, nullptr));
  const char kTraits2[] = R"({
    "trait1": {
      "commands": {
        "command1": {
          "minimalRole": "user",
          "parameters": {"height": {"type": "string"}}
        }
      },
      "state": {
        "property1": {"type": "boolean"}
      }
    },
    "trait3": {
      "state": {
        "property3": {"type": "string"}
      }
    }
  })";
  json = CreateDictionaryValue(kTraits2);
  EXPECT_FALSE(manager.LoadTraits(*json, nullptr));
}

TEST(ComponentManager, LoadTraitsNotAnObject) {
  ComponentManager manager;
  const char kTraits1[] = R"({"trait1": 0})";
  auto json = CreateDictionaryValue(kTraits1);
  ErrorPtr error;
  EXPECT_FALSE(manager.LoadTraits(*json, &error));
  EXPECT_EQ(errors::commands::kTypeMismatch, error->GetCode());
}

TEST(ComponentManager, FindTraitDefinition) {
  ComponentManager manager;
  const char kTraits[] = R"({
    "trait1": {
      "commands": {
        "command1": {
          "minimalRole": "user",
          "parameters": {"height": {"type": "integer"}}
        }
      },
      "state": {
        "property1": {"type": "boolean"}
      }
    },
    "trait2": {
      "state": {
        "property2": {"type": "string"}
      }
    }
  })";
  auto json = CreateDictionaryValue(kTraits);
  ASSERT_TRUE(manager.LoadTraits(*json, nullptr));

  const base::DictionaryValue* trait = manager.FindTraitDefinition("trait1");
  ASSERT_NE(nullptr, trait);
  const char kExpected1[] = R"({
    "commands": {
      "command1": {
        "minimalRole": "user",
        "parameters": {"height": {"type": "integer"}}
      }
    },
    "state": {
      "property1": {"type": "boolean"}
    }
  })";
  EXPECT_JSON_EQ(kExpected1, *trait);

  trait = manager.FindTraitDefinition("trait2");
  ASSERT_NE(nullptr, trait);
  const char kExpected2[] = R"({
    "state": {
      "property2": {"type": "string"}
    }
  })";
  EXPECT_JSON_EQ(kExpected2, *trait);

  EXPECT_EQ(nullptr, manager.FindTraitDefinition("trait3"));
}

TEST(ComponentManager, FindCommandDefinition) {
  ComponentManager manager;
  const char kTraits[] = R"({
    "trait1": {
      "commands": {
        "command1": {
          "minimalRole": "user",
          "parameters": {"height": {"type": "integer"}}
        }
      }
    },
    "trait2": {
      "commands": {
        "command1": {
          "minimalRole": "manager"
        },
        "command2": {
          "minimalRole": "owner"
        }
      }
    }
  })";
  auto json = CreateDictionaryValue(kTraits);
  ASSERT_TRUE(manager.LoadTraits(*json, nullptr));

  const auto* cmd_def = manager.FindCommandDefinition("trait1.command1");
  ASSERT_NE(nullptr, cmd_def);
  const char kExpected1[] = R"({
    "minimalRole": "user",
    "parameters": {"height": {"type": "integer"}}
  })";
  EXPECT_JSON_EQ(kExpected1, *cmd_def);

  cmd_def = manager.FindCommandDefinition("trait2.command1");
  ASSERT_NE(nullptr, cmd_def);
  const char kExpected2[] = R"({
    "minimalRole": "manager"
  })";
  EXPECT_JSON_EQ(kExpected2, *cmd_def);

  cmd_def = manager.FindCommandDefinition("trait2.command2");
  ASSERT_NE(nullptr, cmd_def);
  const char kExpected3[] = R"({
    "minimalRole": "owner"
  })";
  EXPECT_JSON_EQ(kExpected3, *cmd_def);

  EXPECT_EQ(nullptr, manager.FindTraitDefinition("trait1.command2"));
  EXPECT_EQ(nullptr, manager.FindTraitDefinition("trait3.command1"));
  EXPECT_EQ(nullptr, manager.FindTraitDefinition("trait"));
  EXPECT_EQ(nullptr, manager.FindTraitDefinition("trait1.command1.parameters"));
}

TEST(ComponentManager, GetMinimalRole) {
  ComponentManager manager;
  const char kTraits[] = R"({
    "trait1": {
      "commands": {
        "command1": { "minimalRole": "user" },
        "command2": { "minimalRole": "viewer" }
      }
    },
    "trait2": {
      "commands": {
        "command1": { "minimalRole": "manager" },
        "command2": { "minimalRole": "owner" }
      }
    }
  })";
  auto json = CreateDictionaryValue(kTraits);
  ASSERT_TRUE(manager.LoadTraits(*json, nullptr));

  UserRole role;
  ASSERT_TRUE(manager.GetMinimalRole("trait1.command1", &role, nullptr));
  EXPECT_EQ(UserRole::kUser, role);

  ASSERT_TRUE(manager.GetMinimalRole("trait1.command2", &role, nullptr));
  EXPECT_EQ(UserRole::kViewer, role);

  ASSERT_TRUE(manager.GetMinimalRole("trait2.command1", &role, nullptr));
  EXPECT_EQ(UserRole::kManager, role);

  ASSERT_TRUE(manager.GetMinimalRole("trait2.command2", &role, nullptr));
  EXPECT_EQ(UserRole::kOwner, role);

  EXPECT_FALSE(manager.GetMinimalRole("trait1.command3", &role, nullptr));
}

TEST(ComponentManager, AddComponent) {
  ComponentManager manager;
  const char kTraits[] = R"({"trait1": {}, "trait2": {}, "trait3": {}})";
  auto json = CreateDictionaryValue(kTraits);
  ASSERT_TRUE(manager.LoadTraits(*json, nullptr));
  EXPECT_TRUE(manager.AddComponent("", "comp1", {"trait1", "trait2"}, nullptr));
  EXPECT_TRUE(manager.AddComponent("", "comp2", {"trait3"}, nullptr));
  const char kExpected[] = R"({
    "comp1": {
      "traits": ["trait1", "trait2"]
    },
    "comp2": {
      "traits": ["trait3"]
    }
  })";
  EXPECT_JSON_EQ(kExpected, manager.GetComponents());

  // 'trait4' is undefined, so can't add a component referring to it.
  EXPECT_FALSE(manager.AddComponent("", "comp3", {"trait4"}, nullptr));
}

TEST(ComponentManager, AddSubComponent) {
  ComponentManager manager;
  EXPECT_TRUE(manager.AddComponent("", "comp1", {}, nullptr));
  EXPECT_TRUE(manager.AddComponent("comp1", "comp2", {}, nullptr));
  EXPECT_TRUE(manager.AddComponent("comp1", "comp3", {}, nullptr));
  EXPECT_TRUE(manager.AddComponent("comp1.comp2", "comp4", {}, nullptr));
  const char kExpected[] = R"({
    "comp1": {
      "traits": [],
      "components": {
        "comp2": {
          "traits": [],
          "components": {
            "comp4": {
              "traits": []
            }
          }
        },
        "comp3": {
          "traits": []
        }
      }
    }
  })";
  EXPECT_JSON_EQ(kExpected, manager.GetComponents());
}

TEST(ComponentManager, AddComponentArrayItem) {
  ComponentManager manager;
  const char kTraits[] = R"({"foo": {}, "bar": {}})";
  auto json = CreateDictionaryValue(kTraits);
  ASSERT_TRUE(manager.LoadTraits(*json, nullptr));

  EXPECT_TRUE(manager.AddComponent("", "comp1", {}, nullptr));
  EXPECT_TRUE(manager.AddComponentArrayItem("comp1", "comp2", {"foo"},
                                            nullptr));
  EXPECT_TRUE(manager.AddComponentArrayItem("comp1", "comp2", {"bar"},
                                            nullptr));
  EXPECT_TRUE(manager.AddComponent("comp1.comp2[1]", "comp3", {}, nullptr));
  EXPECT_TRUE(manager.AddComponent("comp1.comp2[1].comp3", "comp4", {},
                                   nullptr));
  const char kExpected[] = R"({
    "comp1": {
      "traits": [],
      "components": {
        "comp2": [
          {
            "traits": ["foo"]
          },
          {
            "traits": ["bar"],
            "components": {
              "comp3": {
                "traits": [],
                "components": {
                  "comp4": {
                    "traits": []
                  }
                }
              }
            }
          }
        ]
      }
    }
  })";
  EXPECT_JSON_EQ(kExpected, manager.GetComponents());
}

TEST(ComponentManager, AddComponentExist) {
  ComponentManager manager;
  EXPECT_TRUE(manager.AddComponent("", "comp1", {}, nullptr));
  EXPECT_FALSE(manager.AddComponent("", "comp1", {}, nullptr));
  EXPECT_TRUE(manager.AddComponent("comp1", "comp2", {}, nullptr));
  EXPECT_FALSE(manager.AddComponent("comp1", "comp2", {}, nullptr));
}

TEST(ComponentManager, AddComponentDoesNotExist) {
  ComponentManager manager;
  EXPECT_FALSE(manager.AddComponent("comp1", "comp2", {}, nullptr));
}

TEST(ComponentManager, FindComponent) {
  ComponentManager manager;
  const char kTraits[] = R"({"t1":{}, "t2":{}, "t3":{}, "t4":{}, "t5":{}})";
  auto json = CreateDictionaryValue(kTraits);
  ASSERT_TRUE(manager.LoadTraits(*json, nullptr));

  EXPECT_TRUE(manager.AddComponent("", "comp1", {"t1"}, nullptr));
  EXPECT_TRUE(manager.AddComponentArrayItem("comp1", "comp2", {"t2"}, nullptr));
  EXPECT_TRUE(manager.AddComponentArrayItem("comp1", "comp2", {"t3"}, nullptr));
  EXPECT_TRUE(manager.AddComponent("comp1.comp2[1]", "comp3", {"t4"}, nullptr));
  EXPECT_TRUE(manager.AddComponent("comp1.comp2[1].comp3", "comp4", {"t5"},
                                   nullptr));

  const base::DictionaryValue* comp = manager.FindComponent("comp1", nullptr);
  ASSERT_NE(nullptr, comp);
  EXPECT_TRUE(HasTrait(*comp, "t1"));

  comp = manager.FindComponent("comp1.comp2[0]", nullptr);
  ASSERT_NE(nullptr, comp);
  EXPECT_TRUE(HasTrait(*comp, "t2"));

  comp = manager.FindComponent("comp1.comp2[1]", nullptr);
  ASSERT_NE(nullptr, comp);
  EXPECT_TRUE(HasTrait(*comp, "t3"));

  comp = manager.FindComponent("comp1.comp2[1].comp3", nullptr);
  ASSERT_NE(nullptr, comp);
  EXPECT_TRUE(HasTrait(*comp, "t4"));

  comp = manager.FindComponent("comp1.comp2[1].comp3.comp4", nullptr);
  ASSERT_NE(nullptr, comp);
  EXPECT_TRUE(HasTrait(*comp, "t5"));

  // Some whitespaces don't hurt.
  comp = manager.FindComponent(" comp1 . comp2 [  \t 1 ] .   comp3.comp4 ",
                               nullptr);
  EXPECT_NE(nullptr, comp);

  // Now check some failure cases.
  ErrorPtr error;
  EXPECT_EQ(nullptr, manager.FindComponent("", &error));
  EXPECT_NE(nullptr, error.get());
  // 'comp2' doesn't exist:
  EXPECT_EQ(nullptr, manager.FindComponent("comp2", nullptr));
  // 'comp1.comp2' is an array, not a component:
  EXPECT_EQ(nullptr, manager.FindComponent("comp1.comp2", nullptr));
  // 'comp1.comp2[3]' doesn't exist:
  EXPECT_EQ(nullptr, manager.FindComponent("comp1.comp2[3]", nullptr));
  // Empty component names:
  EXPECT_EQ(nullptr, manager.FindComponent(".comp2[1]", nullptr));
  EXPECT_EQ(nullptr, manager.FindComponent("comp1.[1]", nullptr));
  // Invalid array indices:
  EXPECT_EQ(nullptr, manager.FindComponent("comp1.comp2[s]", nullptr));
  EXPECT_EQ(nullptr, manager.FindComponent("comp1.comp2[-2]", nullptr));
  EXPECT_EQ(nullptr, manager.FindComponent("comp1.comp2[1e1]", nullptr));
  EXPECT_EQ(nullptr, manager.FindComponent("comp1.comp2[1", nullptr));
}

TEST(ComponentManager, AddCommand) {
  ComponentManager manager;
  const char kTraits[] = R"({
    "trait1": {
      "commands": {
        "command1": { "minimalRole": "user" },
        "command2": { "minimalRole": "viewer" }
      }
    },
    "trait2": {
      "commands": {
        "command1": { "minimalRole": "manager" },
        "command2": { "minimalRole": "owner" }
      }
    }
  })";
  auto traits = CreateDictionaryValue(kTraits);
  ASSERT_TRUE(manager.LoadTraits(*traits, nullptr));
  ASSERT_TRUE(manager.AddComponent("", "comp1", {"trait1"}, nullptr));
  ASSERT_TRUE(manager.AddComponent("", "comp2", {"trait2"}, nullptr));

  std::string id;
  const char kCommand1[] = R"({
    "name": "trait1.command1",
    "component": "comp1",
    "parameters": {}
  })";
  auto command1 = CreateDictionaryValue(kCommand1);
  EXPECT_TRUE(manager.AddCommand(*command1, UserRole::kUser, &id, nullptr));
  // Not enough access rights
  EXPECT_FALSE(manager.AddCommand(*command1, UserRole::kViewer, &id, nullptr));

  const char kCommand2[] = R"({
    "name": "trait1.command3",
    "component": "comp1",
    "parameters": {}
  })";
  auto command2 = CreateDictionaryValue(kCommand2);
  // trait1.command3 doesn't exist
  EXPECT_FALSE(manager.AddCommand(*command2, UserRole::kOwner, &id, nullptr));

  const char kCommand3[] = R"({
    "name": "trait2.command1",
    "component": "comp1",
    "parameters": {}
  })";
  auto command3 = CreateDictionaryValue(kCommand3);
  // Component comp1 doesn't have trait2.
  EXPECT_FALSE(manager.AddCommand(*command3, UserRole::kOwner, &id, nullptr));

  // No component specified, use the first top-level component (comp1)
  const char kCommand4[] = R"({
    "name": "trait1.command1",
    "parameters": {}
  })";
  auto command4 = CreateDictionaryValue(kCommand4);
  EXPECT_TRUE(manager.AddCommand(*command4, UserRole::kOwner, &id, nullptr));
  auto cmd = manager.FindCommand(id);
  ASSERT_NE(nullptr, cmd);
  EXPECT_EQ("comp1", cmd->GetComponent());
}

TEST(ComponentManager, AddCommandHandler) {
  ComponentManager manager;
  const char kTraits[] = R"({
    "trait1": {
      "commands": {
        "command1": { "minimalRole": "user" }
      }
    },
    "trait2": {
      "commands": {
        "command2": { "minimalRole": "user" }
      }
    }
  })";
  auto traits = CreateDictionaryValue(kTraits);
  ASSERT_TRUE(manager.LoadTraits(*traits, nullptr));
  ASSERT_TRUE(manager.AddComponent("", "comp1", {"trait1"}, nullptr));
  ASSERT_TRUE(manager.AddComponent("", "comp2", {"trait1", "trait2"}, nullptr));

  std::string last_tags;
  auto handler = [&last_tags](int tag, const std::weak_ptr<Command>& command) {
    if (!last_tags.empty())
      last_tags += ',';
    last_tags += std::to_string(tag);
  };

  manager.AddCommandHandler("comp1", "trait1.command1", base::Bind(handler, 1));
  manager.AddCommandHandler("comp2", "trait1.command1", base::Bind(handler, 2));
  manager.AddCommandHandler("comp2", "trait2.command2", base::Bind(handler, 3));
  EXPECT_TRUE(last_tags.empty());

  const char kCommand1[] = R"({
    "name": "trait1.command1",
    "component": "comp1"
  })";
  auto command1 = CreateDictionaryValue(kCommand1);
  EXPECT_TRUE(manager.AddCommand(*command1, UserRole::kUser, nullptr, nullptr));
  EXPECT_EQ("1", last_tags);
  last_tags.clear();

  const char kCommand2[] = R"({
    "name": "trait1.command1",
    "component": "comp2"
  })";
  auto command2 = CreateDictionaryValue(kCommand2);
  EXPECT_TRUE(manager.AddCommand(*command2, UserRole::kUser, nullptr, nullptr));
  EXPECT_EQ("2", last_tags);
  last_tags.clear();

  const char kCommand3[] = R"({
    "name": "trait2.command2",
    "component": "comp2",
    "parameters": {}
  })";
  auto command3 = CreateDictionaryValue(kCommand3);
  EXPECT_TRUE(manager.AddCommand(*command3, UserRole::kUser, nullptr, nullptr));
  EXPECT_EQ("3", last_tags);
  last_tags.clear();
}

}  // namespace weave
