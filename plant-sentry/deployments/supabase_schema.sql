-- ============================================================
-- PlantSense — Supabase Database Schema
-- ============================================================
-- Enable extensions
create extension if not exists "uuid-ossp";
create extension if not exists "pgcrypto";

-- ============================================================
-- 1. users
-- Managed by Supabase Auth (auth.users).
-- We keep a public profile table for app-level metadata.
-- ============================================================
create table public.profiles (
  id            uuid primary key references auth.users(id) on delete cascade,
  display_name  text,
  avatar_url    text,
  timezone      text default 'Asia/Ho_Chi_Minh',
  created_at    timestamptz default now()
);

-- RLS: each user can only see/edit their own profile
alter table public.profiles enable row level security;
create policy "Users see own profile"
  on public.profiles for select using (auth.uid() = id);
create policy "Users update own profile"
  on public.profiles for update using (auth.uid() = id);

-- Trigger: auto-create profile when user signs up
create or replace function public.handle_new_user()
returns trigger language plpgsql security definer as $$
begin
  insert into public.profiles (id, display_name)
  values (new.id, new.raw_user_meta_data->>'full_name');
  return new;
end;
$$;

create trigger on_auth_user_created
  after insert on auth.users
  for each row execute procedure public.handle_new_user();


-- ============================================================
-- 2. devices
-- Physical ESP32 sensors. One device can manage multiple pots.
-- ============================================================
create table public.devices (
  id               uuid primary key default uuid_generate_v4(),
  user_id          uuid not null references public.profiles(id) on delete cascade,
  device_mac       text unique not null,         -- MAC address e.g. "AA:BB:CC:DD:EE:FF"
  name             text not null,                -- friendly label e.g. "Balcony Sensor"
  location         text,                         -- physical area e.g. "Ban công"
  firmware_version text,
  is_online        boolean default false,
  last_seen        timestamptz,
  created_at       timestamptz default now()
);

alter table public.devices enable row level security;
create policy "Users manage own devices"
  on public.devices for all using (auth.uid() = user_id);

create index idx_devices_user_id on public.devices(user_id);


-- ============================================================
-- 3. plant_pots
-- Each row is one physical plant container monitored by an app.
-- ============================================================
create table public.plant_pots (
  id                  uuid primary key default uuid_generate_v4(),
  user_id             uuid not null references public.profiles(id) on delete cascade,
  device_id           uuid references public.devices(id) on delete set null,
  name                text not null,
  plant_type          text not null,
  emoji               text not null default '🌿',
  location            text not null,              -- "Ban công", "Phòng khách", etc.
  auto_water_enabled  boolean default false,
  water_threshold     int default 30              -- moisture % below which auto-water triggers
                      check (water_threshold between 10 and 60),
  created_at          timestamptz default now(),
  updated_at          timestamptz default now()
);

alter table public.plant_pots enable row level security;
create policy "Users manage own pots"
  on public.plant_pots for all using (auth.uid() = user_id);

create index idx_plant_pots_user_id on public.plant_pots(user_id);
create index idx_plant_pots_device_id on public.plant_pots(device_id);

-- Auto-update updated_at
create or replace function public.set_updated_at()
returns trigger language plpgsql as $$
begin
  new.updated_at = now();
  return new;
end;
$$;

create trigger plant_pots_updated_at
  before update on public.plant_pots
  for each row execute procedure public.set_updated_at();


-- ============================================================
-- 4. sensor_readings
-- Time-series data from ESP32: moisture, temperature, light.
-- Insert-only; never updated.
-- For high-volume production, consider Timescale hypertable
-- or partitioning by month.
-- ============================================================
create table public.sensor_readings (
  id           bigint generated always as identity primary key,
  pot_id       uuid not null references public.plant_pots(id) on delete cascade,
  moisture     numeric(5,2) check (moisture between 0 and 100),
  temperature  numeric(5,2),                      -- °C
  light_level  numeric(5,2) check (light_level between 0 and 100),
  recorded_at  timestamptz not null default now()
);

alter table public.sensor_readings enable row level security;
-- Users can read readings for pots they own
create policy "Users read own readings"
  on public.sensor_readings for select
  using (
    exists (
      select 1 from public.plant_pots
      where id = pot_id and user_id = auth.uid()
    )
  );
-- Devices (service role) can insert readings
create policy "Service role insert readings"
  on public.sensor_readings for insert
  with check (true);  -- restrict further with service_role key at API level

create index idx_sensor_readings_pot_time
  on public.sensor_readings(pot_id, recorded_at desc);

-- Helper view: latest reading per pot
create or replace view public.latest_sensor_readings as
select distinct on (pot_id)
  pot_id, moisture, temperature, light_level, recorded_at
from public.sensor_readings
order by pot_id, recorded_at desc;


-- ============================================================
-- 5. watering_logs
-- Records every watering event (manual, auto, scheduled).
-- ============================================================
create type public.watering_trigger as enum ('manual', 'auto', 'scheduled');

create table public.watering_logs (
  id               bigint generated always as identity primary key,
  pot_id           uuid not null references public.plant_pots(id) on delete cascade,
  triggered_by     public.watering_trigger not null default 'manual',
  duration_seconds int,                          -- how long the valve was open
  moisture_before  numeric(5,2),                 -- snapshot at trigger time
  moisture_after   numeric(5,2),                 -- snapshot ~5 min after
  created_at       timestamptz default now()
);

alter table public.watering_logs enable row level security;
create policy "Users read own watering logs"
  on public.watering_logs for select
  using (
    exists (
      select 1 from public.plant_pots
      where id = pot_id and user_id = auth.uid()
    )
  );
create policy "Service role insert watering logs"
  on public.watering_logs for insert with check (true);

create index idx_watering_logs_pot_time
  on public.watering_logs(pot_id, created_at desc);


-- ============================================================
-- 6. automation_rules
-- Conditions and actions per pot (e.g. water when moisture < 30%).
-- ============================================================
create type public.rule_condition as enum ('moisture_below', 'moisture_above', 'temperature_above', 'light_below');
create type public.rule_action    as enum ('water', 'notify', 'water_and_notify');

create table public.automation_rules (
  id               uuid primary key default uuid_generate_v4(),
  pot_id           uuid not null references public.plant_pots(id) on delete cascade,
  condition_type   public.rule_condition not null,
  threshold_value  numeric(5,2) not null,
  action           public.rule_action not null default 'water',
  is_active        boolean default true,
  created_at       timestamptz default now()
);

alter table public.automation_rules enable row level security;
create policy "Users manage own rules"
  on public.automation_rules for all
  using (
    exists (
      select 1 from public.plant_pots
      where id = pot_id and user_id = auth.uid()
    )
  );

create index idx_automation_rules_pot on public.automation_rules(pot_id);


-- ============================================================
-- 7. notifications
-- In-app notifications generated by automation rules or system events.
-- ============================================================
create type public.notification_type as enum ('dry', 'critical', 'wet', 'info', 'system');

create table public.notifications (
  id          bigint generated always as identity primary key,
  user_id     uuid not null references public.profiles(id) on delete cascade,
  pot_id      uuid references public.plant_pots(id) on delete set null,
  type        public.notification_type not null,
  title       text not null,
  body        text,
  is_read     boolean default false,
  created_at  timestamptz default now()
);

alter table public.notifications enable row level security;
create policy "Users see own notifications"
  on public.notifications for select using (auth.uid() = user_id);
create policy "Users update own notifications"
  on public.notifications for update using (auth.uid() = user_id);
create policy "Service role insert notifications"
  on public.notifications for insert with check (true);

create index idx_notifications_user_unread
  on public.notifications(user_id, is_read, created_at desc);


-- ============================================================
-- 8. Realtime
-- Enable Supabase Realtime for live sensor updates in the app.
-- ============================================================
alter publication supabase_realtime
  add table public.sensor_readings,
            public.notifications,
            public.plant_pots;
