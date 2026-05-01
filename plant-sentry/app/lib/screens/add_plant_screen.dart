import 'package:flutter/material.dart';

import '../models/plant_pot.dart';
import '../theme/app_colors.dart';

class AddPlantScreen extends StatefulWidget {
  const AddPlantScreen({super.key});

  @override
  State<AddPlantScreen> createState() => _AddPlantScreenState();
}

class _AddPlantScreenState extends State<AddPlantScreen> {
  final _formKey = GlobalKey<FormState>();

  final _nameController = TextEditingController();
  final _plantTypeController = TextEditingController();

  String _selectedEmoji = '🌿';
  String _selectedLocation = 'Ban công';
  bool _autoWaterEnabled = false;

  static const _emojis = [
    '🌿',
    '🌵',
    '🌺',
    '🌻',
    '🌹',
    '🌷',
    '🍀',
    '🌱',
    '🌾',
    '🍃',
    '🌴',
    '🪴',
    '🎋',
    '🎍',
    '🍄',
    '🌸',
  ];

  static const _locations = [
    'Ban công',
    'Phòng khách',
    'Phòng ngủ',
    'Cửa sổ',
    'Sân thượng',
    'Nhà bếp',
    'Phòng làm việc',
    'Sân vườn',
  ];

  @override
  void dispose() {
    _nameController.dispose();
    _plantTypeController.dispose();
    super.dispose();
  }

  void _submit() {
    if (!_formKey.currentState!.validate()) return;

    final newPot = PlantPot(
      id: DateTime.now().millisecondsSinceEpoch.toString(),
      name: _nameController.text.trim(),
      plantType: _plantTypeController.text.trim(),
      emoji: _selectedEmoji,
      location: _selectedLocation,
      moisture: 50,
      temperature: 25,
      lightLevel: 60,
      status: MoistureStatus.optimal,
      moistureHistory: [50, 50, 50, 50, 50, 50, 50],
      lastWatered: DateTime.now(),
      autoWaterEnabled: _autoWaterEnabled,
    );

    Navigator.pop(context, newPot);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Thêm chậu mới'),
        backgroundColor: AppColors.darkGreen,
        foregroundColor: Colors.white,
        elevation: 0,
        actions: [
          TextButton(
            onPressed: _submit,
            child: const Text(
              'Lưu',
              style: TextStyle(
                color: Colors.white,
                fontWeight: FontWeight.bold,
                fontSize: 16,
              ),
            ),
          ),
        ],
      ),
      body: Align(
        alignment: Alignment.topCenter,
        child: ConstrainedBox(
          constraints: const BoxConstraints(maxWidth: 620),
          child: Form(
            key: _formKey,
            child: ListView(
              padding: const EdgeInsets.all(16),
              children: [
                // Emoji picker
                _SectionHeader(title: 'Biểu tượng'),
                const SizedBox(height: 10),
                _EmojiPicker(
                  emojis: _emojis,
                  selected: _selectedEmoji,
                  onSelected: (e) => setState(() => _selectedEmoji = e),
                ),
                const SizedBox(height: 20),

                // Name
                _SectionHeader(title: 'Thông tin chậu'),
                const SizedBox(height: 10),
                TextFormField(
                  controller: _nameController,
                  decoration: _inputDecoration(
                    label: 'Tên chậu',
                    hint: 'VD: Chậu ban công A',
                    icon: Icons.local_florist_outlined,
                  ),
                  validator: (v) => (v == null || v.trim().isEmpty)
                      ? 'Vui lòng nhập tên chậu'
                      : null,
                  textCapitalization: TextCapitalization.words,
                ),
                const SizedBox(height: 12),
                TextFormField(
                  controller: _plantTypeController,
                  decoration: _inputDecoration(
                    label: 'Loại cây',
                    hint: 'VD: Cây bạc hà, Xương rồng',
                    icon: Icons.eco_outlined,
                  ),
                  validator: (v) => (v == null || v.trim().isEmpty)
                      ? 'Vui lòng nhập loại cây'
                      : null,
                  textCapitalization: TextCapitalization.sentences,
                ),
                const SizedBox(height: 20),

                // Location
                _SectionHeader(title: 'Vị trí đặt chậu'),
                const SizedBox(height: 10),
                _LocationSelector(
                  locations: _locations,
                  selected: _selectedLocation,
                  onSelected: (loc) => setState(() => _selectedLocation = loc),
                ),
                const SizedBox(height: 20),

                // Auto-water
                _SectionHeader(title: 'Tưới tự động'),
                const SizedBox(height: 8),
                Container(
                  decoration: BoxDecoration(
                    color: Theme.of(context).colorScheme.surface,
                    borderRadius: BorderRadius.circular(12),
                    border: Border.all(color: Colors.grey.shade200),
                  ),
                  child: SwitchListTile(
                    value: _autoWaterEnabled,
                    onChanged: (v) => setState(() => _autoWaterEnabled = v),
                    title: const Text(
                      'Bật tưới tự động',
                      style: TextStyle(fontWeight: FontWeight.w500),
                    ),
                    subtitle: Text(
                      _autoWaterEnabled
                          ? 'Tự động tưới khi độ ẩm thấp'
                          : 'Tắt — chỉ tưới thủ công',
                      style: TextStyle(
                        fontSize: 12,
                        color: Colors.grey.shade500,
                      ),
                    ),
                    activeColor: AppColors.green,
                    shape: RoundedRectangleBorder(
                      borderRadius: BorderRadius.circular(12),
                    ),
                  ),
                ),
                const SizedBox(height: 32),

                // Submit
                FilledButton.icon(
                  onPressed: _submit,
                  icon: const Icon(Icons.add),
                  label: const Text('Thêm chậu cây'),
                  style: FilledButton.styleFrom(
                    backgroundColor: AppColors.green,
                    foregroundColor: Colors.white,
                    padding: const EdgeInsets.symmetric(vertical: 14),
                    shape: RoundedRectangleBorder(
                      borderRadius: BorderRadius.circular(12),
                    ),
                    textStyle: const TextStyle(
                      fontSize: 16,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                ),
                const SizedBox(height: 24),
              ],
            ),
          ),
        ),
      ),
    );
  }

  InputDecoration _inputDecoration({
    required String label,
    required String hint,
    required IconData icon,
  }) {
    return InputDecoration(
      labelText: label,
      hintText: hint,
      prefixIcon: Icon(icon, size: 20),
      filled: true,
      fillColor: Theme.of(context).colorScheme.surface,
      border: OutlineInputBorder(
        borderRadius: BorderRadius.circular(12),
        borderSide: BorderSide(color: Colors.grey.shade200),
      ),
      enabledBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(12),
        borderSide: BorderSide(color: Colors.grey.shade200),
      ),
      focusedBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(12),
        borderSide: const BorderSide(color: AppColors.green, width: 1.5),
      ),
    );
  }
}

// ─── Section header ───────────────────────────────────────────────────────────

class _SectionHeader extends StatelessWidget {
  final String title;
  const _SectionHeader({required this.title});

  @override
  Widget build(BuildContext context) {
    return Text(
      title,
      style: const TextStyle(
        fontSize: 14,
        fontWeight: FontWeight.bold,
        color: AppColors.darkGreen,
      ),
    );
  }
}

// ─── Emoji picker ─────────────────────────────────────────────────────────────

class _EmojiPicker extends StatelessWidget {
  final List<String> emojis;
  final String selected;
  final ValueChanged<String> onSelected;

  const _EmojiPicker({
    required this.emojis,
    required this.selected,
    required this.onSelected,
  });

  @override
  Widget build(BuildContext context) {
    return GridView.builder(
      shrinkWrap: true,
      physics: const NeverScrollableScrollPhysics(),
      gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
        crossAxisCount: 8,
        mainAxisSpacing: 8,
        crossAxisSpacing: 8,
        childAspectRatio: 1,
      ),
      itemCount: emojis.length,
      itemBuilder: (_, i) {
        final emoji = emojis[i];
        final isSelected = emoji == selected;
        return GestureDetector(
          onTap: () => onSelected(emoji),
          child: AnimatedContainer(
            duration: const Duration(milliseconds: 150),
            decoration: BoxDecoration(
              color: isSelected
                  ? AppColors.green.withValues(alpha: 0.15)
                  : Theme.of(context).colorScheme.surfaceContainerHighest,
              borderRadius: BorderRadius.circular(10),
              border: Border.all(
                color: isSelected ? AppColors.green : Colors.transparent,
                width: 2,
              ),
            ),
            child: Center(
              child: Text(emoji, style: const TextStyle(fontSize: 22)),
            ),
          ),
        );
      },
    );
  }
}

// ─── Location selector ────────────────────────────────────────────────────────

class _LocationSelector extends StatelessWidget {
  final List<String> locations;
  final String selected;
  final ValueChanged<String> onSelected;

  const _LocationSelector({
    required this.locations,
    required this.selected,
    required this.onSelected,
  });

  @override
  Widget build(BuildContext context) {
    return Wrap(
      spacing: 8,
      runSpacing: 8,
      children: locations.map((loc) {
        final isSelected = loc == selected;
        return GestureDetector(
          onTap: () => onSelected(loc),
          child: AnimatedContainer(
            duration: const Duration(milliseconds: 150),
            padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 8),
            decoration: BoxDecoration(
              color: isSelected
                  ? AppColors.green
                  : Theme.of(context).colorScheme.surface,
              borderRadius: BorderRadius.circular(20),
              border: Border.all(
                color: isSelected ? AppColors.green : Colors.grey.shade300,
              ),
            ),
            child: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                Icon(
                  Icons.location_on_outlined,
                  size: 14,
                  color: isSelected ? Colors.white : Colors.grey.shade600,
                ),
                const SizedBox(width: 4),
                Text(
                  loc,
                  style: TextStyle(
                    fontSize: 13,
                    fontWeight: FontWeight.w500,
                    color: isSelected ? Colors.white : Colors.grey.shade700,
                  ),
                ),
              ],
            ),
          ),
        );
      }).toList(),
    );
  }
}
