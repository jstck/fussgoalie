#include <avr/io.h>

#define MAX_GOAL_TIME 1.0   // Max time in seconds for a goal to be considered (float)
#define MIN_GOAL_INTERVAL 1  // Minimum time in seconds between goals (integer)

#define GOAL_A_INDEX 0
#define GOAL_B_INDEX 1

#define GOAL_A_PIN_FRONT 18
#define GOAL_A_PIN_BACK  19
#define GOAL_B_PIN_FRONT 20
#define GOAL_B_PIN_BACK  21

#define GOAL_A_INT_FRONT 5
#define GOAL_A_INT_BACK 4
#define GOAL_B_INT_FRONT 3
#define GOAL_B_INT_BACK 2

#define LOOP_DELAY 100

struct Goal {
  long time_front;  // us
  long time_back;  // us
  long time_last_score;  // us
  float velocity_m_s;  // m/s
  float velocity_km_h;  // km/h

  const float distance;  // cm
  const char *color;
};

// Global variables
boolean g_time_last_score = 0;

Goal g_goals[2] = {
  {0, 0, 0, 0.0, 0.0, 0.0326, "yellow"},
  {0, 0, 0, 0.0, 0.0, 0.0339, "black"}
};

// Goal A interrupts
void goal_a_int_front() {
  g_goals[GOAL_A_INDEX].time_front = micros();
}

void goal_a_int_back() {
  g_goals[GOAL_A_INDEX].time_back = micros();
  score_goal(GOAL_A_INDEX);
}

// Goal B interrupts
void goal_b_int_front() {
  g_goals[GOAL_B_INDEX].time_front = micros();
}

void goal_b_int_back() {
  g_goals[GOAL_B_INDEX].time_back = micros();
  score_goal(GOAL_B_INDEX);
}

// Score a goal (called from interrupt)
void score_goal(int index) {
  Goal &goal = g_goals[index];
  float time_diff;  // seconds

  // Do not count if last goal was too recent (bouncing ball)
  if((goal.time_last_score - g_time_last_score) < (1000000 * MIN_GOAL_INTERVAL)) {
    return;
  }

  // Do not count if the front sensor has not triggered yet
  if(goal.time_front == 0) {
    return;
  }

  // ms -> s
  time_diff = 0.000001 * (goal.time_back - goal.time_front);

  if(time_diff > MAX_GOAL_TIME) {
    return;
  }

  goal.velocity_m_s = goal.distance / time_diff;  // m/s
  goal.velocity_km_h = goal.velocity_m_s * 3.6;  // m/s -> km/h
  goal.time_last_score = goal.time_back;
}

// Check if any goal was scored (called from main loop)
void check_goals() {
  for (int index = 0; index < 2; index++) {
    Goal &goal = g_goals[index];

    if (goal.time_last_score > g_time_last_score) {
      Serial.print("GOAL! (color: ");
      Serial.print(goal.color);
      Serial.print(", velocity (m/s): ");
      Serial.print(goal.velocity_m_s);
      Serial.print(", velocity (km/h): ");
      Serial.print(goal.velocity_km_h);
      Serial.print("m/s, time: ");
      Serial.print(goal.time_back - goal.time_front);
      Serial.println("us");

      goal.time_front = 0;
      goal.time_back = 0;
      goal.time_last_score = 0;
      goal.velocity_m_s = 0.0;
      goal.velocity_km_h = 0.0;

      g_time_last_score = micros();
      break;
    }
  }
}

// Setup
void setup() {
  Serial.begin(115200);
  Serial.println("Initializing...");

  noInterrupts();

  pinMode(GOAL_A_PIN_FRONT, INPUT);
  pinMode(GOAL_A_PIN_BACK, INPUT);
  pinMode(GOAL_B_PIN_FRONT, INPUT);
  pinMode(GOAL_B_PIN_BACK, INPUT);

  attachInterrupt(GOAL_A_INT_FRONT, goal_a_int_front, RISING);
  attachInterrupt(GOAL_A_INT_BACK, goal_a_int_back, RISING);
  attachInterrupt(GOAL_B_INT_FRONT, goal_b_int_front, RISING);
  attachInterrupt(GOAL_B_INT_BACK, goal_b_int_back, RISING);

  interrupts();

  Serial.println("Done.");
}

// Main loop
void loop() {
  check_goals();
  delay(LOOP_DELAY);
}
