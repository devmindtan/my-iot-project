-- ============================================================
-- PlantSense - Seed Test Data (Supabase)
-- ============================================================
-- Usage:
-- Run this file in Supabase SQL Editor (no user required).
-- A shared family profile row is created automatically if missing.

begin;

do $$
declare
  v_user_id uuid := '00000000-0000-0000-0000-000000000001'; -- shared family account (no auth)
  v_device_1 uuid;
  v_device_2 uuid;
  v_pot_1 uuid;
  v_pot_2 uuid;
  v_pot_3 uuid;
  i int;
begin
  -- Insert a shared family profile if it does not exist yet.
  -- NOTE: public.profiles has a FK to auth.users. If you have not created
  -- a real auth user, first run the workaround below in Supabase SQL Editor:
  --   ALTER TABLE public.profiles DROP CONSTRAINT profiles_id_fkey;
  -- After that, this seed will work without any auth user.
  insert into public.profiles (id, display_name)
  values (v_user_id, 'Gia dinh')
  on conflict (id) do nothing;

-- 1. Thực hiện Insert/Update trước (Bỏ returning vào biến)
  insert into public.devices (user_id, device_mac, name, location, firmware_version, is_online, last_seen)
  values
    (v_user_id, 'DE:MO:11:22:33:01', 'Demo Sensor Balcony', 'Ban cong', '1.2.0', true, now()),
    (v_user_id, 'DE:MO:11:22:33:02', 'Demo Sensor Living Room', 'Phong khach', '1.2.0', true, now())
  on conflict (device_mac) do update
    set user_id = excluded.user_id,
        name = excluded.name,
        location = excluded.location,
        firmware_version = excluded.firmware_version,
        is_online = excluded.is_online,
        last_seen = excluded.last_seen;

  -- 2. Sau đó mới gán ID vào biến (Đoạn này bạn đã có sẵn)
  select id into v_device_1 from public.devices where device_mac = 'DE:MO:11:22:33:01';
  select id into v_device_2 from public.devices where device_mac = 'DE:MO:11:22:33:02';

  -- 3. Tương tự cho phần plant_pots (Bỏ returning vào biến vì bạn insert 3 dòng)
  insert into public.plant_pots (user_id, device_id, name, plant_type, emoji, location, auto_water_enabled, water_threshold)
  values
    (v_user_id, v_device_1, 'Chau Demo 1', 'Monstera', '🌿', 'Ban cong', true, 30),
    (v_user_id, v_device_1, 'Chau Demo 2', 'Pothos', '🪴', 'Ban cong', false, 30),
    (v_user_id, v_device_2, 'Chau Demo 3', 'Snake Plant', '🌵', 'Phong khach', true, 25);

  -- 4. Lấy ID các chậu cây (Sử dụng SELECT INTO như bạn đã làm ở dưới)
  select id into v_pot_1 from public.plant_pots where user_id = v_user_id and name = 'Chau Demo 1' order by created_at desc limit 1;
  select id into v_pot_2 from public.plant_pots where user_id = v_user_id and name = 'Chau Demo 2' order by created_at desc limit 1;
  select id into v_pot_3 from public.plant_pots where user_id = v_user_id and name = 'Chau Demo 3' order by created_at desc limit 1;

  for i in 0..47 loop
    insert into public.sensor_readings (pot_id, moisture, temperature, light_level, recorded_at)
    values
      (v_pot_1, 25 + random() * 45, 24 + random() * 8, 35 + random() * 55, now() - (i * interval '10 minutes')),
      (v_pot_2, 20 + random() * 55, 23 + random() * 9, 25 + random() * 65, now() - (i * interval '10 minutes')),
      (v_pot_3, 18 + random() * 50, 22 + random() * 10, 20 + random() * 70, now() - (i * interval '10 minutes'));
  end loop;

  insert into public.automation_rules (pot_id, condition_type, threshold_value, action, is_active)
  values
    (v_pot_1, 'moisture_below', 30, 'water_and_notify', true),
    (v_pot_2, 'moisture_below', 28, 'notify', true),
    (v_pot_3, 'moisture_below', 25, 'water', true);

  insert into public.notifications (user_id, pot_id, type, title, body, is_read)
  values
    (v_user_id, v_pot_1, 'dry', 'Chau Demo 1 can tuoi', 'Do am dat dang thap, can kiem tra he thong tuoi.', false),
    (v_user_id, v_pot_3, 'info', 'Thong tin demo', 'Day la thong bao test cho man hinh dashboard.', false);
end $$;

commit;
