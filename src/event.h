extern "C"
{
  enum EventType
  {
    EVENT_TYPE_WINDOW_RESIZE,
    EVENT_TYPE_KEYDOWN,
  };

  enum Key
  {
    KEY_W = 1,
    KEY_S,
    KEY_A,
    KEY_D,
    KEY_COUNT,
  };

  struct WindowResize
  {
    u32 width;
    u32 height;
  };

  struct Keydown
  {
    Key key;
  };

  // TODO(szulf): make this the tagged union
  struct Event
  {
    EventType type;
    union
    {
      WindowResize window_resize;
      Keydown keydown;
    } data;
  };
}
