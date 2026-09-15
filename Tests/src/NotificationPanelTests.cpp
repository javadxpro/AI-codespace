#include <kimia_test.h>
#include <kimia/NotificationPanel.h>

KIMIA_TEST(Notification_DrawEmptyDoesNotCrash) {
  kimia::ui::drawNotificationPanel({0, 0, 240, 80}, {});
}

KIMIA_TEST(Notification_DrawAllKinds) {
  std::vector<kimia::ui::Notification> v;
  v.push_back({"Welcome to the editor", kimia::ui::NotificationKind::Info});
  v.push_back({"Scene has unsaved changes", kimia::ui::NotificationKind::Warning});
  v.push_back({"Failed to compile shader",  kimia::ui::NotificationKind::Error});
  v.push_back({"Saved successfully",       kimia::ui::NotificationKind::Success});
  kimia::ui::drawNotificationPanel({0, 0, 320, 200}, v);
}

KIMIA_TEST(Notification_DrawMany) {
  std::vector<kimia::ui::Notification> v;
  for (int i = 0; i < 10; ++i) {
    kimia::ui::Notification n;
    n.message = "msg " + std::to_string(i);
    n.kind = static_cast<kimia::ui::NotificationKind>(i % 4);
    n.lifetimeSec = static_cast<float>(i);
    v.push_back(n);
  }
  kimia::ui::drawNotificationPanel({0, 0, 280, 300}, v);
}

KIMIA_TEST(Notification_DrawWithLongMessage) {
  std::vector<kimia::ui::Notification> v(1);
  v[0].message = "A_Very_Long_Notification_Message_That_Should_Still_Be_Drawn_Without_Crashing_Or_Overrunning_The_Panel_Width";
  v[0].kind = kimia::ui::NotificationKind::Info;
  kimia::ui::drawNotificationPanel({0, 0, 200, 40}, v);
}

KIMIA_TEST(Notification_DrawAtPhonePortrait) {
  std::vector<kimia::ui::Notification> v;
  v.push_back({"Saved", kimia::ui::NotificationKind::Success});
  v.push_back({"Error", kimia::ui::NotificationKind::Error});
  kimia::ui::drawNotificationPanel({0, 0, 240, 320}, v);
}

KIMIA_TEST(Notification_DrawAtTabletLandscape) {
  std::vector<kimia::ui::Notification> v;
  v.push_back({"Build complete", kimia::ui::NotificationKind::Success});
  kimia::ui::drawNotificationPanel({0, 0, 400, 100}, v);
}

KIMIA_TEST(Notification_StickyLifetimeZeroIsFine) {
  std::vector<kimia::ui::Notification> v(1);
  v[0].message = "Sticky";
  v[0].kind = kimia::ui::NotificationKind::Warning;
  v[0].lifetimeSec = 0.0f;
  kimia::ui::drawNotificationPanel({0, 0, 200, 40}, v);
}
