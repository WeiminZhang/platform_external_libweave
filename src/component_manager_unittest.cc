// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/component_manager_impl.h"

#include <map>

#include <gtest/gtest.h>
#include <weave/test/unittest_utils.h>

#include "src/bind_lambda.h"
#include "src/commands/schema_constants.h"
#include "src/mock_component_manager.h"

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

// Creates sample trait/component trees:
// {
//   "traits": {
//     "t1": {},
//     "t2": {},
//     "t3": {},
//     "t4": {},
//     "t5": {},
//     "t6": {},
//   },
//   "components": {
//     "comp1": {
//       "traits": [ "t1" ],
//       "components": {
//         "comp2": [
//           { "traits": [ "t2" ] },
//           {
//             "traits": [ "t3" ],
//             "components": {
//               "comp3": {
//                 "traits": [ "t4" ],
//                 "components": {
//                   "comp4": {
//                     "traits": [ "t5", "t6" ]
//                   }
//                 }
//               }
//             }
//           }
//         ],
//       }
//     }
//   }
// }
void CreateTestComponentTree(ComponentManager* manager) {
  const char kTraits[] = R"({"t1":{},"t2":{},"t3":{},"t4":{},"t5":{},"t6":{}})";
  auto json = CreateDictionaryValue(kTraits);
  ASSERT_TRUE(manager->LoadTraits(*json, nullptr));
  EXPECT_TRUE(manager->AddComponent("", "comp1", {"t1"}, nullptr));
  EXPECT_TRUE(manager->AddComponentArrayItem("comp1", "comp2", {"t2"},
                                             nullptr));
  EXPECT_TRUE(manager->AddComponentArrayItem("comp1", "comp2", {"t3"},
                                             nullptr));
  EXPECT_TRUE(manager->AddComponent("comp1.comp2[1]", "comp3", {"t4"},
                                    nullptr));
  EXPECT_TRUE(manager->AddComponent("comp1.comp2[1].comp3", "comp4",
                                    {"t5", "t6"}, nullptr));
}

// Test clock class to record predefined time intervals.
// Implementation from base/test/simple_test_clock.{h|cc}
class SimpleTestClock : public base::Clock {
 public:
  base::Time Now() override { return now_; }

  // Advances the clock by |delta|.
  void Advance(base::TimeDelta delta) { now_ += delta; }

  // Sets the clock to the given time.
  void SetNow(base::Time now) { now_ = now; }

 private:
  base::Time now_;
};

}  // anonymous namespace

TEST(ComponentManager, Empty) {
  ComponentManagerImpl manager;
  EXPECT_TRUE(manager.GetTraits().empty());
  EXPECT_TRUE(manager.GetComponents().empty());
}

TEST(ComponentManager, LoadTraits) {
  ComponentManagerImpl manager;
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
  ComponentManagerImpl manager;
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
  ComponentManagerImpl manager;
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

TEST(ComponentManager, AddTraitDefChangedCallback) {
  ComponentManagerImpl manager;
  int count = 0;
  int count2 = 0;
  manager.AddTraitDefChangedCallback(base::Bind([&count]() { count++; }));
  manager.AddTraitDefChangedCallback(base::Bind([&count2]() { count2++; }));
  EXPECT_EQ(1, count);
  EXPECT_EQ(1, count2);
  // New definitions.
  const char kTraits1[] = R"({
    "trait1": {
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
  EXPECT_EQ(2, count);
  // Duplicate definition, shouldn't call the callback.
  const char kTraits2[] = R"({
    "trait1": {
      "state": {
        "property1": {"type": "boolean"}
      }
    }
  })";
  json = CreateDictionaryValue(kTraits2);
  EXPECT_TRUE(manager.LoadTraits(*json, nullptr));
  EXPECT_EQ(2, count);
  // New definition, should call the callback now.
  const char kTraits3[] = R"({
    "trait3": {
      "state": {
        "property3": {"type": "string"}
      }
    }
  })";
  json = CreateDictionaryValue(kTraits3);
  EXPECT_TRUE(manager.LoadTraits(*json, nullptr));
  EXPECT_EQ(3, count);
  // Wrong definition, shouldn't call the callback.
  const char kTraits4[] = R"({
    "trait4": "foo"
  })";
  json = CreateDictionaryValue(kTraits4);
  EXPECT_FALSE(manager.LoadTraits(*json, nullptr));
  EXPECT_EQ(3, count);
  // Make sure both callbacks were called the same number of times.
  EXPECT_EQ(count2, count);
}

TEST(ComponentManager, LoadTraitsNotAnObject) {
  ComponentManagerImpl manager;
  const char kTraits1[] = R"({"trait1": 0})";
  auto json = CreateDictionaryValue(kTraits1);
  ErrorPtr error;
  EXPECT_FALSE(manager.LoadTraits(*json, &error));
  EXPECT_EQ(errors::commands::kTypeMismatch, error->GetCode());
}

TEST(ComponentManager, FindTraitDefinition) {
  ComponentManagerImpl manager;
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
  ComponentManagerImpl manager;
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
  ComponentManagerImpl manager;
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
  ComponentManagerImpl manager;
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
  ComponentManagerImpl manager;
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
  ComponentManagerImpl manager;
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
  ComponentManagerImpl manager;
  EXPECT_TRUE(manager.AddComponent("", "comp1", {}, nullptr));
  EXPECT_FALSE(manager.AddComponent("", "comp1", {}, nullptr));
  EXPECT_TRUE(manager.AddComponent("comp1", "comp2", {}, nullptr));
  EXPECT_FALSE(manager.AddComponent("comp1", "comp2", {}, nullptr));
}

TEST(ComponentManager, AddComponentDoesNotExist) {
  ComponentManagerImpl manager;
  EXPECT_FALSE(manager.AddComponent("comp1", "comp2", {}, nullptr));
}

TEST(ComponentManager, AddComponentTreeChangedCallback) {
  ComponentManagerImpl manager;
  int count = 0;
  int count2 = 0;
  manager.AddComponentTreeChangedCallback(base::Bind([&count]() { count++; }));
  manager.AddComponentTreeChangedCallback(
      base::Bind([&count2]() { count2++; }));
  EXPECT_EQ(1, count);
  EXPECT_EQ(1, count2);
  EXPECT_TRUE(manager.AddComponent("", "comp1", {}, nullptr));
  EXPECT_EQ(2, count);
  EXPECT_TRUE(manager.AddComponent("comp1", "comp2", {}, nullptr));
  EXPECT_EQ(3, count);
  EXPECT_TRUE(manager.AddComponent("comp1.comp2", "comp4", {}, nullptr));
  EXPECT_EQ(4, count);
  EXPECT_TRUE(manager.AddComponentArrayItem("comp1", "comp3", {}, nullptr));
  EXPECT_EQ(5, count);
  EXPECT_TRUE(manager.AddComponentArrayItem("comp1", "comp3", {}, nullptr));
  EXPECT_EQ(6, count);
  // Make sure both callbacks were called the same number of times.
  EXPECT_EQ(count2, count);
}

TEST(ComponentManager, FindComponent) {
  ComponentManagerImpl manager;
  CreateTestComponentTree(&manager);

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
  ComponentManagerImpl manager;
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
    "id": "1234-12345",
    "component": "comp1",
    "parameters": {}
  })";
  auto command1 = CreateDictionaryValue(kCommand1);
  EXPECT_TRUE(manager.AddCommand(*command1, Command::Origin::kLocal,
                                 UserRole::kUser, &id, nullptr));
  EXPECT_EQ("1234-12345", id);
  // Not enough access rights
  EXPECT_FALSE(manager.AddCommand(*command1, Command::Origin::kLocal,
                                  UserRole::kViewer, &id, nullptr));

  const char kCommand2[] = R"({
    "name": "trait1.command3",
    "component": "comp1",
    "parameters": {}
  })";
  auto command2 = CreateDictionaryValue(kCommand2);
  // trait1.command3 doesn't exist
  EXPECT_FALSE(manager.AddCommand(*command2, Command::Origin::kLocal,
                                  UserRole::kOwner, &id, nullptr));
  EXPECT_TRUE(id.empty());

  const char kCommand3[] = R"({
    "name": "trait2.command1",
    "component": "comp1",
    "parameters": {}
  })";
  auto command3 = CreateDictionaryValue(kCommand3);
  // Component comp1 doesn't have trait2.
  EXPECT_FALSE(manager.AddCommand(*command3, Command::Origin::kLocal,
                                  UserRole::kOwner, &id, nullptr));

  // No component specified, find the suitable component
  const char kCommand4[] = R"({
    "name": "trait1.command1",
    "parameters": {}
  })";
  auto command4 = CreateDictionaryValue(kCommand4);
  EXPECT_TRUE(manager.AddCommand(*command4, Command::Origin::kLocal,
                                 UserRole::kOwner, &id, nullptr));
  auto cmd = manager.FindCommand(id);
  ASSERT_NE(nullptr, cmd);
  EXPECT_EQ("comp1", cmd->GetComponent());

  const char kCommand5[] = R"({
    "name": "trait2.command1",
    "parameters": {}
  })";
  auto command5 = CreateDictionaryValue(kCommand5);
  EXPECT_TRUE(manager.AddCommand(*command5, Command::Origin::kLocal,
                                 UserRole::kOwner, &id, nullptr));
  cmd = manager.FindCommand(id);
  ASSERT_NE(nullptr, cmd);
  EXPECT_EQ("comp2", cmd->GetComponent());
}

TEST(ComponentManager, AddCommandHandler) {
  ComponentManagerImpl manager;
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
  EXPECT_TRUE(manager.AddCommand(*command1, Command::Origin::kCloud,
                                 UserRole::kUser, nullptr, nullptr));
  EXPECT_EQ("1", last_tags);
  last_tags.clear();

  const char kCommand2[] = R"({
    "name": "trait1.command1",
    "component": "comp2"
  })";
  auto command2 = CreateDictionaryValue(kCommand2);
  EXPECT_TRUE(manager.AddCommand(*command2, Command::Origin::kCloud,
                                 UserRole::kUser, nullptr, nullptr));
  EXPECT_EQ("2", last_tags);
  last_tags.clear();

  const char kCommand3[] = R"({
    "name": "trait2.command2",
    "component": "comp2",
    "parameters": {}
  })";
  auto command3 = CreateDictionaryValue(kCommand3);
  EXPECT_TRUE(manager.AddCommand(*command3, Command::Origin::kLocal,
                                 UserRole::kUser, nullptr, nullptr));
  EXPECT_EQ("3", last_tags);
  last_tags.clear();
}

TEST(ComponentManager, SetStateProperties) {
  ComponentManagerImpl manager;
  CreateTestComponentTree(&manager);

  const char kState1[] = R"({"t1": {"p1": 0, "p2": "foo"}})";
  auto state1 = CreateDictionaryValue(kState1);
  ASSERT_TRUE(manager.SetStateProperties("comp1", *state1, nullptr));
  const char kExpected1[] = R"({
    "comp1": {
      "traits": [ "t1" ],
      "state": {"t1": {"p1": 0, "p2": "foo"}},
      "components": {
        "comp2": [
          {
            "traits": [ "t2" ]
          },
          {
            "traits": [ "t3" ],
            "components": {
              "comp3": {
                "traits": [ "t4" ],
                "components": {
                  "comp4": {
                    "traits": [ "t5", "t6" ]
                  }
                }
              }
            }
          }
        ]
      }
    }
  })";
  EXPECT_JSON_EQ(kExpected1, manager.GetComponents());

  const char kState2[] = R"({"t1": {"p1": {"bar": "baz"}}})";
  auto state2 = CreateDictionaryValue(kState2);
  ASSERT_TRUE(manager.SetStateProperties("comp1", *state2, nullptr));

  const char kExpected2[] = R"({
    "comp1": {
      "traits": [ "t1" ],
      "state": {"t1": {"p1": {"bar": "baz"}, "p2": "foo"}},
      "components": {
        "comp2": [
          {
            "traits": [ "t2" ]
          },
          {
            "traits": [ "t3" ],
            "components": {
              "comp3": {
                "traits": [ "t4" ],
                "components": {
                  "comp4": {
                    "traits": [ "t5", "t6" ]
                  }
                }
              }
            }
          }
        ]
      }
    }
  })";
  EXPECT_JSON_EQ(kExpected2, manager.GetComponents());

  const char kState3[] = R"({"t5": {"p1": 1}})";
  auto state3 = CreateDictionaryValue(kState3);
  ASSERT_TRUE(manager.SetStateProperties("comp1.comp2[1].comp3.comp4", *state3,
                                         nullptr));

  const char kExpected3[] = R"({
    "comp1": {
      "traits": [ "t1" ],
      "state": {"t1": {"p1": {"bar": "baz"}, "p2": "foo"}},
      "components": {
        "comp2": [
          {
            "traits": [ "t2" ]
          },
          {
            "traits": [ "t3" ],
            "components": {
              "comp3": {
                "traits": [ "t4" ],
                "components": {
                  "comp4": {
                    "traits": [ "t5", "t6" ],
                    "state": { "t5": { "p1": 1 } }
                  }
                }
              }
            }
          }
        ]
      }
    }
  })";
  EXPECT_JSON_EQ(kExpected3, manager.GetComponents());
}

TEST(ComponentManager, SetStatePropertiesFromJson) {
  ComponentManagerImpl manager;
  CreateTestComponentTree(&manager);

  ASSERT_TRUE(manager.SetStatePropertiesFromJson(
      "comp1.comp2[1].comp3.comp4", R"({"t5": {"p1": 3}, "t6": {"p2": 5}})",
      nullptr));

  const char kExpected[] = R"({
    "comp1": {
      "traits": [ "t1" ],
      "components": {
        "comp2": [
          {
            "traits": [ "t2" ]
          },
          {
            "traits": [ "t3" ],
            "components": {
              "comp3": {
                "traits": [ "t4" ],
                "components": {
                  "comp4": {
                    "traits": [ "t5", "t6" ],
                    "state": {
                      "t5": { "p1": 3 },
                      "t6": { "p2": 5 }
                    }
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

TEST(ComponentManager, SetGetStateProperty) {
  ComponentManagerImpl manager;
  const char kTraits[] = R"({
    "trait1": {
      "state": {
        "prop1": { "type": "string" },
        "prop2": { "type": "integer" }
      }
    },
    "trait2": {
      "state": {
        "prop3": { "type": "string" },
        "prop4": { "type": "string" }
      }
    }
  })";
  auto traits = CreateDictionaryValue(kTraits);
  ASSERT_TRUE(manager.LoadTraits(*traits, nullptr));
  ASSERT_TRUE(manager.AddComponent("", "comp1", {"trait1", "trait2"}, nullptr));

  base::StringValue p1("foo");
  ASSERT_TRUE(manager.SetStateProperty("comp1", "trait1.prop1", p1, nullptr));

  const char kExpected1[] = R"({
    "comp1": {
      "traits": [ "trait1", "trait2" ],
      "state": {
        "trait1": { "prop1": "foo" }
      }
    }
  })";
  EXPECT_JSON_EQ(kExpected1, manager.GetComponents());

  base::FundamentalValue p2(2);
  ASSERT_TRUE(manager.SetStateProperty("comp1", "trait2.prop3", p2, nullptr));

  const char kExpected2[] = R"({
    "comp1": {
      "traits": [ "trait1", "trait2" ],
      "state": {
        "trait1": { "prop1": "foo" },
        "trait2": { "prop3": 2 }
      }
    }
  })";
  EXPECT_JSON_EQ(kExpected2, manager.GetComponents());
  // Just the package name without property:
  EXPECT_FALSE(manager.SetStateProperty("comp1", "trait2", p2, nullptr));

  const base::Value* value = manager.GetStateProperty("comp1", "trait1.prop1",
                                                      nullptr);
  ASSERT_NE(nullptr, value);
  EXPECT_TRUE(p1.Equals(value));
  value = manager.GetStateProperty("comp1", "trait2.prop3", nullptr);
  ASSERT_NE(nullptr, value);
  EXPECT_TRUE(p2.Equals(value));

  // Non-existing property:
  EXPECT_EQ(nullptr, manager.GetStateProperty("comp1", "trait2.p", nullptr));
  // Non-existing component
  EXPECT_EQ(nullptr, manager.GetStateProperty("comp2", "trait.prop", nullptr));
  // Just the package name without property:
  EXPECT_EQ(nullptr, manager.GetStateProperty("comp1", "trait2", nullptr));
}

TEST(ComponentManager, AddStateChangedCallback) {
  SimpleTestClock clock;
  ComponentManagerImpl manager{&clock};
  const char kTraits[] = R"({
    "trait1": {
      "state": {
        "prop1": { "type": "string" },
        "prop2": { "type": "string" }
      }
    }
  })";
  auto traits = CreateDictionaryValue(kTraits);
  ASSERT_TRUE(manager.LoadTraits(*traits, nullptr));
  ASSERT_TRUE(manager.AddComponent("", "comp1", {"trait1"}, nullptr));

  int count = 0;
  int count2 = 0;
  manager.AddStateChangedCallback(base::Bind([&count]() { count++; }));
  manager.AddStateChangedCallback(base::Bind([&count2]() { count2++; }));
  EXPECT_EQ(1, count);
  EXPECT_EQ(1, count2);
  EXPECT_EQ(0, manager.GetLastStateChangeId());

  base::StringValue p1("foo");
  ASSERT_TRUE(manager.SetStateProperty("comp1", "trait1.prop1", p1, nullptr));
  EXPECT_EQ(2, count);
  EXPECT_EQ(2, count2);
  EXPECT_EQ(1, manager.GetLastStateChangeId());

  ASSERT_TRUE(manager.SetStateProperty("comp1", "trait1.prop2", p1, nullptr));
  EXPECT_EQ(3, count);
  EXPECT_EQ(3, count2);
  EXPECT_EQ(2, manager.GetLastStateChangeId());

  // Fail - no component.
  ASSERT_FALSE(manager.SetStateProperty("comp2", "trait1.prop2", p1, nullptr));
  EXPECT_EQ(3, count);
  EXPECT_EQ(3, count2);
  EXPECT_EQ(2, manager.GetLastStateChangeId());
}

TEST(ComponentManager, ComponentStateUpdates) {
  SimpleTestClock clock;
  ComponentManagerImpl manager{&clock};
  const char kTraits[] = R"({
    "trait1": {
      "state": {
        "prop1": { "type": "string" },
        "prop2": { "type": "string" }
      }
    },
    "trait2": {
      "state": {
        "prop3": { "type": "string" },
        "prop4": { "type": "string" }
      }
    }
  })";
  auto traits = CreateDictionaryValue(kTraits);
  ASSERT_TRUE(manager.LoadTraits(*traits, nullptr));
  ASSERT_TRUE(manager.AddComponent("", "comp1", {"trait1", "trait2"}, nullptr));
  ASSERT_TRUE(manager.AddComponent("", "comp2", {"trait1", "trait2"}, nullptr));

  std::vector<ComponentManager::UpdateID> updates1;
  auto callback1 = [&updates1](ComponentManager::UpdateID id) {
    updates1.push_back(id);
  };
  // State change queue is empty, callback should be called immediately.
  auto token1 = manager.AddServerStateUpdatedCallback(base::Bind(callback1));
  ASSERT_EQ(1u, updates1.size());
  EXPECT_EQ(manager.GetLastStateChangeId(), updates1.front());
  updates1.clear();

  base::StringValue foo("foo");
  base::Time time1 = base::Time::Now();
  clock.SetNow(time1);
  // These three updates should be grouped into two separate state change queue
  // items, since they all happen at the same time, but for two different
  // components.
  ASSERT_TRUE(manager.SetStateProperty("comp1", "trait1.prop1", foo, nullptr));
  ASSERT_TRUE(manager.SetStateProperty("comp2", "trait2.prop3", foo, nullptr));
  ASSERT_TRUE(manager.SetStateProperty("comp1", "trait1.prop2", foo, nullptr));

  std::vector<ComponentManager::UpdateID> updates2;
  auto callback2 = [&updates2](ComponentManager::UpdateID id) {
    updates2.push_back(id);
  };
  // State change queue is not empty, so callback will be called later.
  auto token2 = manager.AddServerStateUpdatedCallback(base::Bind(callback2));
  EXPECT_TRUE(updates2.empty());

  base::StringValue bar("bar");
  base::Time time2 = time1 + base::TimeDelta::FromSeconds(1);
  clock.SetNow(time2);
  // Two more update events (as above) but at |time2|.
  ASSERT_TRUE(manager.SetStateProperty("comp1", "trait1.prop1", bar, nullptr));
  ASSERT_TRUE(manager.SetStateProperty("comp2", "trait2.prop3", bar, nullptr));
  ASSERT_TRUE(manager.SetStateProperty("comp1", "trait1.prop2", bar, nullptr));

  auto snapshot = manager.GetAndClearRecordedStateChanges();
  EXPECT_EQ(manager.GetLastStateChangeId(), snapshot.update_id);
  ASSERT_EQ(4u, snapshot.state_changes.size());

  EXPECT_EQ("comp1", snapshot.state_changes[0].component);
  EXPECT_EQ(time1, snapshot.state_changes[0].timestamp);
  EXPECT_JSON_EQ(R"({"trait1":{"prop1":"foo","prop2":"foo"}})",
                 *snapshot.state_changes[0].changed_properties);

  EXPECT_EQ("comp2", snapshot.state_changes[1].component);
  EXPECT_EQ(time1, snapshot.state_changes[1].timestamp);
  EXPECT_JSON_EQ(R"({"trait2":{"prop3":"foo"}})",
                 *snapshot.state_changes[1].changed_properties);

  EXPECT_EQ("comp1", snapshot.state_changes[2].component);
  EXPECT_EQ(time2, snapshot.state_changes[2].timestamp);
  EXPECT_JSON_EQ(R"({"trait1":{"prop1":"bar","prop2":"bar"}})",
                 *snapshot.state_changes[2].changed_properties);

  EXPECT_EQ("comp2", snapshot.state_changes[3].component);
  EXPECT_EQ(time2, snapshot.state_changes[3].timestamp);
  EXPECT_JSON_EQ(R"({"trait2":{"prop3":"bar"}})",
                 *snapshot.state_changes[3].changed_properties);

  // Make sure previous GetAndClearRecordedStateChanges() clears the queue.
  auto snapshot2 = manager.GetAndClearRecordedStateChanges();
  EXPECT_EQ(manager.GetLastStateChangeId(), snapshot2.update_id);
  EXPECT_TRUE(snapshot2.state_changes.empty());

  // Now indicate that we have update the changes on the server.
  manager.NotifyStateUpdatedOnServer(snapshot.update_id);
  ASSERT_EQ(1u, updates1.size());
  EXPECT_EQ(snapshot.update_id, updates1.front());
  ASSERT_EQ(1u, updates2.size());
  EXPECT_EQ(snapshot.update_id, updates2.front());
}

TEST(ComponentManager, FindComponentWithTrait) {
  ComponentManagerImpl manager;
  const char kTraits[] = R"({
    "trait1": {},
    "trait2": {},
    "trait3": {}
  })";
  auto traits = CreateDictionaryValue(kTraits);
  ASSERT_TRUE(manager.LoadTraits(*traits, nullptr));
  ASSERT_TRUE(manager.AddComponent("", "comp1", {"trait1", "trait2"}, nullptr));
  ASSERT_TRUE(manager.AddComponent("", "comp2", {"trait3"}, nullptr));

  EXPECT_EQ("comp1", manager.FindComponentWithTrait("trait1"));
  EXPECT_EQ("comp1", manager.FindComponentWithTrait("trait2"));
  EXPECT_EQ("comp2", manager.FindComponentWithTrait("trait3"));
  EXPECT_EQ("", manager.FindComponentWithTrait("trait4"));
}

TEST(ComponentManager, TestMockComponentManager) {
  // Check that all the virtual methods are mocked out.
  MockComponentManager mock;
}

}  // namespace weave
