/**
 * @file test_lcb_frame_config.cpp
 * @brief Unit tests for the Layer_1 capabilities frame_config module
 */

#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "frame_config.h"
#include "lcb_type.h"
}

TEST(FrameConfigTest, RecumbentBikePerformance_MatchesJavaLiterals) {
    frame_config_t cfg;
    frame_config_init_recumbent_bike_performance(&cfg);

    EXPECT_STREQ(cfg.frame_prefix, "RB317");
    EXPECT_EQ(cfg.frame_id, 103);
    EXPECT_STREQ(cfg.model, "PERFORMANCE");

    ASSERT_EQ(cfg.support_lcb_type_count, 1);
    EXPECT_EQ(cfg.support_lcb_types[0], LCB_TYPE_LCBA);

    EXPECT_FLOAT_EQ(cfg.speed_mph_min_model, 0.0f);

    EXPECT_FLOAT_EQ(cfg.resistance_min_model, 1.0f);
    EXPECT_FLOAT_EQ(cfg.resistance_max_model, 30.0f);

    EXPECT_NEAR(cfg.weight_kg_min_model, 22.6796185f, 0.001f);
    EXPECT_NEAR(cfg.weight_kg_max_model, 181.436948f, 0.001f);
    EXPECT_FLOAT_EQ(cfg.weight_lbs_min_model, 50.0f);
    EXPECT_FLOAT_EQ(cfg.weight_lbs_max_model, 400.0f);

    EXPECT_FLOAT_EQ(cfg.rate_resistance_per_sec, 5.0f);
    EXPECT_TRUE(cfg.lcb_can_control_watts);
}

TEST(FrameConfigTest, Init_NullSafe) {
    frame_config_init_recumbent_bike_performance(NULL);
}

TEST(FrameConfigTest, SupportsLcbType_TrueForListedType) {
    frame_config_t cfg;
    frame_config_init_recumbent_bike_performance(&cfg);
    EXPECT_TRUE(frame_config_supports_lcb_type(&cfg, LCB_TYPE_LCBA));
}

TEST(FrameConfigTest, SupportsLcbType_FalseForUnlistedType) {
    frame_config_t cfg;
    frame_config_init_recumbent_bike_performance(&cfg);
    EXPECT_FALSE(frame_config_supports_lcb_type(&cfg, LCB_TYPE_1X_BIKE_LCB));
    EXPECT_FALSE(frame_config_supports_lcb_type(&cfg, LCB_TYPE_UNKNOWN));
}

TEST(FrameConfigTest, SupportsLcbType_NullConfig_ReturnsFalse) {
    EXPECT_FALSE(frame_config_supports_lcb_type(NULL, LCB_TYPE_LCBA));
}
