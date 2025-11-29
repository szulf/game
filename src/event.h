extern "C"
{
  enum EventType
  {
    EVENT_TYPE_WINDOW_RESIZE,
  };

  enum Key
  {
    KEY_W = 1,
    KEY_S,
    KEY_A,
    KEY_D,
    KEY_F1,
    KEY_SPACE,
    KEY_LSHIFT,
    KEY_COUNT,
  };

  struct WindowResize
  {
    u32 width;
    u32 height;
  };

  struct Event
  {
    EventType type;
    union
    {
      WindowResize window_resize;
    } data;
  };
}
