#include "YamlParserTest.h"

TEST_F(YamlParserTest, calcAutopasBox) {
  // tests the AutoBox calculation after Object initialization with multipleObjectsWithMultipleTypeTest
  MDFlexConfig config;
  config.yamlFilename = "multipleObjectsWithMultipleTypesTest.yaml";
  parser.parseYamlFile(config);
  config.calcSimulationBox();
  std::array<double, 3> compBoxMin = {0, -15, -15};
  std::array<double, 3> compBoxMax = {23, 10, 13};
  EXPECT_EQ(config.boxMin, compBoxMin);
  EXPECT_EQ(config.boxMax, compBoxMax);
}

TEST_F(YamlParserTest, addType) {
  // tests if exception are thrown if types arent well initialized
  MDFlexConfig config;
  config.addParticleType(0, 1.0, 1.0, 1.0);
  EXPECT_NO_THROW(config.addParticleType(0, 1.0, 1.0, 1.0));
  EXPECT_ANY_THROW(config.addParticleType(0, 1.5, 1.0, 1.0));
  EXPECT_ANY_THROW(config.addParticleType(0, 1.5, 1.1, 1.0));
  EXPECT_ANY_THROW(config.addParticleType(0, 1.1, 1.1, 1.1));
  EXPECT_NO_THROW(config.addParticleType(1, 2.0, 1.0, 1.0));
  EXPECT_EQ(config.massMap.at(0), 1.0);
  EXPECT_EQ(config.massMap.at(1), 1.0);
  EXPECT_EQ(config.epsilonMap.at(1), 2.0);
}

TEST_F(YamlParserTest, wrongTypeParsingInput) {
  MDFlexConfig config;
  config.yamlFilename = "incorrectParsingFile.yaml";
  ASSERT_ANY_THROW(parser.parseYamlFile(config));
}

TEST_F(YamlParserTest, multipleSameObjectParsing) {
  MDFlexConfig config;
  config.yamlFilename = "multipleSimilarObjects.yaml";
  parser.parseYamlFile(config);
  ASSERT_EQ(config.cubeGridObjects.size(), 2);
  ASSERT_EQ(config.cubeGridObjects.at(0).getTypeId(), 0);
  ASSERT_EQ(config.cubeGridObjects.at(0).getParticleSpacing(), 0.5);
  ASSERT_EQ(config.cubeGridObjects.at(1).getTypeId(), 1);
}