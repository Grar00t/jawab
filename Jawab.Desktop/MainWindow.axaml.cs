using System;
using System.IO;
using System.Linq;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Layout;
using Avalonia.Media;
using Avalonia.Platform.Storage;

namespace Jawab.Desktop;

public partial class MainWindow : Window
{
    private bool _isArabic;
    private string? _lastImportedFile;

    public MainWindow()
    {
        InitializeComponent();
        ApplyEnglish();
        UpdateSendButton();
    }

    private async void ImportButton_Click(object? sender, RoutedEventArgs e)
    {
        var files = await StorageProvider.OpenFilePickerAsync(new FilePickerOpenOptions
        {
            Title = _isArabic ? "استيراد ملف محلي" : "Import a local file",
            AllowMultiple = false,
            FileTypeFilter =
            [
                new FilePickerFileType(_isArabic ? "ملفات نصية وMarkdown" : "Text and Markdown files")
                {
                    Patterns = ["*.txt", "*.md", "*.markdown"]
                }
            ]
        });

        var file = files.FirstOrDefault();
        if (file is null)
            return;

        _lastImportedFile = file.Path.LocalPath;
        var name = Path.GetFileName(_lastImportedFile);

        IndexStatusText.Text = _isArabic
            ? $"الملف المحدد: {name}"
            : $"Selected: {name}";

        AddSystemMessage(_isArabic ? $"تم اختيار الملف: {name}" : $"Selected file: {name}");
    }

    private void SendButton_Click(object? sender, RoutedEventArgs e) => SendQuestion();

    private void QuestionBox_KeyDown(object? sender, KeyEventArgs e)
    {
        if (e.Key == Key.Enter)
        {
            e.Handled = true;
            SendQuestion();
        }
    }

    private void QuestionBox_TextChanged(object? sender, TextChangedEventArgs e) => UpdateSendButton();

    private void UpdateSendButton()
    {
        SendButton.IsEnabled = true;
    }

    private void SendQuestion()
    {
        var question = QuestionBox.Text?.Trim();

        if (string.IsNullOrWhiteSpace(question))
            return;

        AddUserMessage(question);
        QuestionBox.Text = string.Empty;
        UpdateSendButton();

        AddSystemMessage(
            string.IsNullOrWhiteSpace(_lastImportedFile)
                ? (_isArabic ? "استورد ملفًا قبل البحث." : "Import a file before searching.")
                : (_isArabic ? "تم استلام السؤال." : "Question received."));
    }

    private void SaveNoteButton_Click(object? sender, RoutedEventArgs e)
    {
        var note = QuestionBox.Text?.Trim();

        if (string.IsNullOrWhiteSpace(note))
        {
            AddSystemMessage(_isArabic ? "اكتب ملاحظة أولًا." : "Write a note first.");
            return;
        }

        var notesFolder = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
            "Jawab",
            "Notes");

        Directory.CreateDirectory(notesFolder);

        var fileName = $"note-{DateTime.Now:yyyyMMdd-HHmmss}.md";
        var filePath = Path.Combine(notesFolder, fileName);

        var noteTitle = _isArabic ? "ملاحظة" : "Note";
        var content = $"# {noteTitle}{Environment.NewLine}{Environment.NewLine}{note}{Environment.NewLine}";
        File.WriteAllText(filePath, content);

        IndexStatusText.Text = _isArabic
            ? $"تم حفظ ملاحظة: {fileName}"
            : $"Saved note: {fileName}";

        QuestionBox.Text = string.Empty;
        UpdateSendButton();

        AddSystemMessage(
            _isArabic
                ? $"تم حفظ الملاحظة محليًا: {fileName}"
                : $"Note saved locally: {fileName}");
    }

    private async void LanguageButton_Click(object? sender, RoutedEventArgs e)
    {
        var dialog = new Window
        {
            Title = _isArabic ? "اللغة" : "Language",
            Width = 380,
            Height = 250,
            CanResize = false,
            WindowStartupLocation = WindowStartupLocation.CenterOwner,
            Background = Brush.Parse("#F8FAFC")
        };

        var title = new TextBlock
        {
            Text = _isArabic ? "اختر لغة الواجهة" : "Choose interface language",
            FontSize = 18,
            FontWeight = FontWeight.SemiBold,
            Foreground = Brush.Parse("#0F172A")
        };

        var english = new Button
        {
            Content = "English",
            MinHeight = 44,
            HorizontalContentAlignment = HorizontalAlignment.Left
        };

        var arabic = new Button
        {
            Content = "العربية",
            MinHeight = 44,
            HorizontalContentAlignment = HorizontalAlignment.Right,
            FlowDirection = FlowDirection.RightToLeft
        };

        var close = new Button
        {
            Content = _isArabic ? "إغلاق" : "Close",
            HorizontalAlignment = HorizontalAlignment.Right,
            MinWidth = 88
        };

        english.Click += (_, _) =>
        {
            ApplyEnglish();
            dialog.Close();
        };

        arabic.Click += (_, _) =>
        {
            ApplyArabic();
            dialog.Close();
        };

        close.Click += (_, _) => dialog.Close();

        dialog.Content = new Border
        {
            Padding = new Thickness(24),
            Child = new StackPanel
            {
                Spacing = 10,
                Children = { title, english, arabic, close }
            }
        };

        await dialog.ShowDialog(this);
    }

    private async void AboutButton_Click(object? sender, RoutedEventArgs e)
    {
        var dialog = CreateInfoDialog(
            "About Jawab",
            _isArabic ? "حول Jawab" : "About Jawab",
            _isArabic
                ? "أداة محلية لتنظيم الملفات والبحث في محتواها مع إظهار المصدر."
                : "A local tool for organizing files, searching their contents, and showing the source.");

        await dialog.ShowDialog(this);
    }

    private async void SettingsButton_Click(object? sender, RoutedEventArgs e)
    {
        var dialog = CreateInfoDialog(
            _isArabic ? "إعدادات Jawab" : "Jawab Settings",
            _isArabic ? "الإعدادات" : "Settings",
            _isArabic
                ? "ستظهر هنا خيارات التخزين المحلي والفهرسة والواجهة."
                : "Local storage, indexing, and interface options will appear here.");

        await dialog.ShowDialog(this);
    }

    private Window CreateInfoDialog(string title, string heading, string message)
    {
        var dialog = new Window
        {
            Title = title,
            Width = 520,
            Height = 300,
            MinWidth = 420,
            MinHeight = 240,
            WindowStartupLocation = WindowStartupLocation.CenterOwner,
            Background = Brush.Parse("#F8FAFC")
        };

        var close = new Button
        {
            Content = _isArabic ? "إغلاق" : "Close",
            HorizontalAlignment = HorizontalAlignment.Right,
            MinWidth = 90
        };

        close.Click += (_, _) => dialog.Close();

        dialog.Content = new Border
        {
            Padding = new Thickness(28),
            Child = new StackPanel
            {
                Spacing = 14,
                Children =
                {
                    new TextBlock
                    {
                        Text = heading,
                        FontSize = 24,
                        FontWeight = FontWeight.SemiBold,
                        Foreground = Brush.Parse("#0F172A"),
                        FlowDirection = _isArabic ? FlowDirection.RightToLeft : FlowDirection.LeftToRight
                    },
                    new TextBlock
                    {
                        Text = message,
                        FontSize = 15,
                        TextWrapping = TextWrapping.Wrap,
                        Foreground = Brush.Parse("#334155"),
                        FlowDirection = _isArabic ? FlowDirection.RightToLeft : FlowDirection.LeftToRight,
                        TextAlignment = _isArabic ? TextAlignment.Right : TextAlignment.Left
                    },
                    close
                }
            }
        };

        return dialog;
    }

    private void ApplyEnglish()
    {
        _isArabic = false;
        FlowDirection = FlowDirection.LeftToRight;

        HeaderSubtitle.Text = "Local evidence search";
        ImportButton.Content = "Import files";
        LanguageButton.Content = "Language ▾";

        KnowledgeBasesLabel.Text = "Knowledge bases";
        AllDataButton.Content = "All local data";
        TechnicalButton.Content = "Technical corpus";
        NotesButton.Content = "Notes";
        NewKnowledgeButton.Content = "New knowledge base";
        StatusLabel.Text = "Status";

        PageTitle.Text = "Ask your knowledge";
        PageDescription.Text = "Search imported files and inspect the evidence behind every result.";
        WelcomeNoteText.Text = "Import a TXT or Markdown file to begin.";

        QuestionBox.FlowDirection = FlowDirection.LeftToRight;
        QuestionBox.TextAlignment = TextAlignment.Left;
        QuestionBox.PlaceholderText = "Ask a question…";
        SendButton.Content = "➤";
        SaveNoteButton.Content = "⌑";
    }

    private void ApplyArabic()
    {
        _isArabic = true;
        FlowDirection = FlowDirection.RightToLeft;

        HeaderSubtitle.Text = "بحث محلي قائم على الدليل";
        ImportButton.Content = "استيراد ملفات";
        LanguageButton.Content = "اللغة ▾";

        KnowledgeBasesLabel.Text = "قواعد المعرفة";
        AllDataButton.Content = "كل البيانات المحلية";
        TechnicalButton.Content = "المجموعة التقنية";
        NotesButton.Content = "ملاحظات";
        NewKnowledgeButton.Content = "قاعدة معرفة جديدة";
        StatusLabel.Text = "الحالة";

        PageTitle.Text = "اسأل معرفتك";
        PageDescription.Text = "ابحث في الملفات المستوردة وافحص المصدر وراء كل نتيجة.";
        WelcomeNoteText.Text = "استورد ملف TXT أو Markdown للبدء.";

        QuestionBox.FlowDirection = FlowDirection.RightToLeft;
        QuestionBox.TextAlignment = TextAlignment.Right;
        QuestionBox.PlaceholderText = "اكتب سؤالًا…";
        SendButton.Content = "➤";
        SaveNoteButton.Content = "⌑";
    }

    private void AddUserMessage(string message)
    {
        var panel = new StackPanel
        {
            HorizontalAlignment = _isArabic ? HorizontalAlignment.Left : HorizontalAlignment.Right,
            MaxWidth = 700,
            Spacing = 5
        };

        panel.Children.Add(new TextBlock
        {
            Text = _isArabic ? "أنت" : "You",
            FontSize = 12,
            FontWeight = FontWeight.SemiBold,
            Foreground = Brush.Parse("#64748B"),
            HorizontalAlignment = _isArabic ? HorizontalAlignment.Left : HorizontalAlignment.Right
        });

        panel.Children.Add(new Border
        {
            Background = Brush.Parse("#0EA5E9"),
            CornerRadius = new CornerRadius(16),
            Padding = new Thickness(16, 12),
            Child = new TextBlock
            {
                Text = message,
                FontSize = 16,
                Foreground = Brushes.White,
                TextWrapping = TextWrapping.Wrap,
                FlowDirection = _isArabic ? FlowDirection.RightToLeft : FlowDirection.LeftToRight,
                TextAlignment = _isArabic ? TextAlignment.Right : TextAlignment.Left
            }
        });

        ConversationPanel.Children.Add(panel);
        ConversationScroll.ScrollToEnd();
    }

    private void AddSystemMessage(string message)
    {
        var panel = new StackPanel
        {
            HorizontalAlignment = HorizontalAlignment.Left,
            MaxWidth = 820,
            Spacing = 7
        };

        panel.Children.Add(new TextBlock
        {
            Text = "jawab",
            FontSize = 12,
            FontWeight = FontWeight.SemiBold,
            Foreground = Brush.Parse("#64748B")
        });

        panel.Children.Add(new Border
        {
            Background = Brushes.White,
            BorderBrush = Brush.Parse("#E5E7EB"),
            BorderThickness = new Thickness(1),
            CornerRadius = new CornerRadius(16),
            Padding = new Thickness(18),
            Child = new TextBlock
            {
                Text = message,
                FontSize = 15,
                Foreground = Brush.Parse("#1E293B"),
                TextWrapping = TextWrapping.Wrap,
                FlowDirection = _isArabic ? FlowDirection.RightToLeft : FlowDirection.LeftToRight,
                TextAlignment = _isArabic ? TextAlignment.Right : TextAlignment.Left
            }
        });

        ConversationPanel.Children.Add(panel);
        ConversationScroll.ScrollToEnd();
    }
}






