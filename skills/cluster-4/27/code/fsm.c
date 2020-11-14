// Alex Prior
// 10/27/2020

typedef enum {			// Set of states enumerated
       MOLE_UP_STATE,
       MOLE_DOWN_STATE,
       MAX_STATES
       } state_e;

typedef enum {			// Set of events enumerated
	TIME_DELAY_EVENT,
	MOLE_HIT_EVENT,
	MOLE_NOT_HIT_EVENT,
	MAX_EVENTS
	} event_e;

state_e state = MOLE_DOWN_STATE;	// Starting state
state_e next_state;
event_e event;

while (1)			// When event occurs, event handler does some stuff
{
	event = read_event();
	if (state == MOLE_DOWN_STATE)
	{
		if (event == TIME_DELAY_STATE)
		{
			next_state == time_delay_event_handler();
		}
        else if (event == MOLE_HIT_EVENT)
        {
            next_state == mole_hit_event_handler();
        }
        else if (event == MOLE_NOT_HIT_EVENT)
        {
            next_state == mole_not_hit_event_handler();
        }
    }
	else if (state == MOLE_UP_STATE)
	{
		if (event == TIME_DELAY_STATE)
		{
			next_state == time_delay_event_handler();
		}
        else if (event == MOLE_HIT_EVENT)
        {
            next_state == mole_hit_event_handler();
        }
        else if (event == MOLE_NOT_HIT_EVENT)
        {
            next_state == mole_not_hit_event_handler();
        }
    }
}
	state = next_state;