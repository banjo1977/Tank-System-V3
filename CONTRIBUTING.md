## 👩‍💻 How to Contribute to *Tank-System-V3* Using Visual Studio

*(Beginner-friendly guide for new GitHub users)*

---

### 🧠 Understanding GitHub: Basic Concepts

If you're new to GitHub, here are the essential terms you'll see during the process:

| Term             | What it Means                                                                   |
| ---------------- | ------------------------------------------------------------------------------- |
| **Clone**        | Make a copy of the GitHub project on your computer so you can work on it.       |
| **Branch**       | A separate workspace to make changes without affecting the main project.        |
| **Commit**       | A saved version of your code changes with a short message describing them.      |
| **Push**         | Send your commits (saved changes) from your computer to GitHub.                 |
| **Pull Request** | A request to add your changes into the original project. Reviewed by the owner. |

---

### ✅ Step 1: Install Prerequisites

Make sure you have:

* **Visual Studio 2022** or later
  (with the **.NET Desktop Development** workload installed, if applicable)
* A **GitHub account** (sign up at [github.com](https://github.com) if you don’t have one)

---

### ✅ Step 2: Clone the Project from GitHub

1. Open **Visual Studio**

2. Go to **File > Clone Repository**

3. In the **Repository location**, paste:

   ```
   https://github.com/banjo1977/Tank-System-V3.git
   ```

4. Choose a **local folder** to store the project

5. Click **"Clone"**

---

### ✅ Step 3: Create a New Branch for Your Changes

1. Open the **Git Changes** window (`View > Git Changes`)
2. Click on the **current branch name** (probably `main`) at the bottom right
3. Click **"New Branch"**, give it a name like `fix-layout` or `feature-xyz`
4. Click **"Create Branch"**, then **"Checkout"** to switch to it

---

### ✅ Step 4: Make Your Changes

* Open the project in **Solution Explorer**
* Edit the code or UI as needed
* Press **F5** to test your changes

💡 **Tip:** Save often and make small, focused changes.

---

### ✅ Step 5: Commit Your Changes

1. Go to the **Git Changes** window
2. Enter a short description of what you changed (e.g., "Fix tank level display bug")
3. Click **"Commit All"**

---

### ✅ Step 6: Push Your Branch to GitHub

After committing:

1. Click **"Push"** in the Git Changes window
2. This uploads your changes to your **GitHub fork**

---

### ✅ Step 7: Create a Pull Request

1. Go to [https://github.com/banjo1977/Tank-System-V3](https://github.com/banjo1977/Tank-System-V3)
2. GitHub should show a prompt: **"Compare & pull request"**
3. Click it, write a short explanation of what you changed, and submit it

Banjo (the project owner) will review and merge it if everything looks good!

---

### ❓ Need Help?

If you're stuck, you can:

* Ask a question by creating an **Issue** on the GitHub repo
* Or message the project owner
